#include <unistd.h>
#include "asf_sar.h"
#include "asf_raster.h"
#include "asf_complex.h"
#include <assert.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_eigen.h>

typedef struct {
   int current_row;
   int nrows;  // # in held in memory, not total image rows
   meta_parameters *meta;

   float *amp; // HH amplitude data

   quadPolFloat *data_buffer;
   quadPolFloat **lines;

   complexVector *pauli_buffer;
   complexVector **pauli_lines;

   complexMatrix **coh_buffer;
   complexMatrix ***coh_lines;

   int amp_band;
   int hh_amp_band, hh_phase_band;
   int hv_amp_band, hv_phase_band;
   int vh_amp_band, vh_phase_band;
   int vv_amp_band, vv_phase_band;
} PolarimetricImageRows;


//static complexFloat complex_new_gsl(gsl_complex c)
//{
//    complexFloat ret;
//    ret.real = GSL_REAL(c);
//    ret.imag = GSL_IMAG(c);
//    return ret;
//}

static quadPolFloat qual_pol_zero()
{
    quadPolFloat ret;
    ret.hh = complex_zero();
    ret.vh = complex_zero();
    ret.hv = complex_zero();
    ret.vv = complex_zero();
    return ret;
}

static PolarimetricImageRows *
polarimetric_image_rows_new(meta_parameters *meta, int nrows, int multi)
{
    PolarimetricImageRows *self = MALLOC(sizeof(PolarimetricImageRows));

    self->nrows = nrows;
    self->meta = meta;

    // nrows must be odd
    if (multi) {
      self->current_row = 0;
    }
    else {
      assert((self->nrows-1)%2==0);
      self->current_row = -(nrows+1)/2;
    }

    int ns = meta->general->sample_count;

    self->amp = CALLOC(ns, sizeof(float));
    self->data_buffer = CALLOC(nrows*ns, sizeof(quadPolFloat));
    self->lines = CALLOC(nrows, sizeof(quadPolFloat*));

    // initially, the line pointers point at their natural locations in
    // the buffer
    int i;
    for (i=0; i<nrows; ++i)
        self->lines[i] = &(self->data_buffer[ns*i]);

    // these guys are the pauli basis elements we've calculated for the
    // loaded rows
    self->pauli_buffer = CALLOC(nrows*ns, sizeof(complexVector));
    self->pauli_lines = CALLOC(nrows, sizeof(complexVector*));
    for (i=0; i<nrows; ++i)
        self->pauli_lines[i] = &(self->pauli_buffer[ns*i]);

    // coherency matrix elements for the loaded rows
    self->coh_buffer = MALLOC(nrows*ns*sizeof(complexMatrix*));
    for (i=0; i<nrows*ns; ++i)
        self->coh_buffer[i] = complex_matrix_new(3,3);
    self->coh_lines = MALLOC(nrows*sizeof(complexMatrix**));
    for (i=0; i<nrows; ++i)
        self->coh_lines[i] = &(self->coh_buffer[ns*i]);

    // band numbers in the input file
    self->amp_band = -1;
    self->hh_amp_band = self->hh_phase_band = -1;
    self->hv_amp_band = self->hv_phase_band = -1;
    self->vh_amp_band = self->vh_phase_band = -1;
    self->vv_amp_band = self->vv_phase_band = -1;

    return self;
}

static int polarimetric_image_rows_get_bands(PolarimetricImageRows *self)
{
    int ok=TRUE;
    // "extra" amplitude band needs to be the first band
    char *bands = self->meta->general->bands;
    self->amp_band = strncmp_case(bands, "AMP", 3) == 0 ? 0 : -1;
    // polarimetric bands
    self->hh_amp_band = find_band(self->meta, "AMP-HH", &ok);
    self->hh_phase_band = find_band(self->meta, "PHASE-HH", &ok);
    self->hv_amp_band = find_band(self->meta, "AMP-HV", &ok);
    self->hv_phase_band = find_band(self->meta, "PHASE-HV", &ok);
    self->vh_amp_band = find_band(self->meta, "AMP-VH", &ok);
    self->vh_phase_band = find_band(self->meta, "PHASE-VH", &ok);
    self->vv_amp_band = find_band(self->meta, "AMP-VV", &ok);
    self->vv_phase_band = find_band(self->meta, "PHASE-VV", &ok);
    return ok;
}

static void calculate_pauli_for_row(PolarimetricImageRows *self, int n)
{
    int j, ns=self->meta->general->sample_count;
    for (j=0; j<ns; ++j) {
        quadPolFloat q = self->lines[n][j];

        // HH-VV, HV+VH, HH+VV
        self->pauli_lines[n][j] =
          complex_vector_new(
	    complex_sub(q.hh, q.vv),
	    complex_add(q.hv, q.vh),
	    complex_add(q.hh, q.vv));
    }
}

static void calculate_coherence_for_row(PolarimetricImageRows *self, int n)
{
    // [ A*A  B*A  C*A ]    A = HH + VV
    // [ A*B  B*B  C*B ]    B = HH - VV
    // [ A*C  B*C  C*C ]    C = 2*HV
    int j, ns=self->meta->general->sample_count;
    for (j=0; j<ns; ++j) {
        quadPolFloat q = self->lines[n][j];
        complexVector v = //complex_vector_normalize(
          complex_vector_new(
            complex_add(q.hh, q.vv),
            complex_sub(q.hh, q.vv),
            complex_add(q.hv, q.vh))/*)*/;
        complexVector vc = complex_vector_conj(v);

        complexMatrix *m = self->coh_lines[n][j];

        complex_matrix_set(m,0,0,complex_mul(vc.A, v.A));
        complex_matrix_set(m,0,1,complex_mul(vc.A, v.B));
        complex_matrix_set(m,0,2,complex_mul(vc.A, v.C));

        complex_matrix_set(m,1,0,complex_mul(vc.B, v.A));
        complex_matrix_set(m,1,1,complex_mul(vc.B, v.B));
        complex_matrix_set(m,1,2,complex_mul(vc.B, v.C));

        complex_matrix_set(m,2,0,complex_mul(vc.C, v.A));
        complex_matrix_set(m,2,1,complex_mul(vc.C, v.B));
        complex_matrix_set(m,2,2,complex_mul(vc.C, v.C));

        //complex_matrix_scale(m,1./sqrt(2.0));
        complex_matrix_scale(m,0.5);
    }
}

static void polarimetric_image_rows_load_next_row(PolarimetricImageRows *self,
                                                  FILE *fin)
{
  // we discard the top (0) row, slide all rows up one, then load
  // the new row into the top position

  // don't actually move any data -- update pointers into the
  // buffers

  // FIRST -- slide row pointers
  int k;
  for (k=0; k<self->nrows-1; ++k) {
    self->lines[k] = self->lines[k+1];
    self->pauli_lines[k] = self->pauli_lines[k+1];
  }
  
  // the next line to load will go into the spot we just dumped
  int last = self->nrows - 1;
  self->lines[last] = self->lines[0];
  self->pauli_lines[last] = self->pauli_lines[0];
  
  self->current_row++;
  
  // NEXT, load in new row into the final row
  // if we have moved off the top of the image, we will need to
  // fill with zeros, instead of loading a row
  int ns = self->meta->general->sample_count;
  float *amp_buf = MALLOC(sizeof(float)*ns);
  float *phase_buf = MALLOC(sizeof(float)*ns);

  int row = self->current_row + (self->nrows-1)/2;
  if (row < self->meta->general->line_count) {
    // amplutide, we only store the current row
    if (self->current_row >= 0 && self->amp_band >= 0)
      get_band_float_line(fin, self->meta, self->amp_band,
                          self->current_row, self->amp);

    // now the SLC rows
    get_band_float_line(fin, self->meta, self->hh_amp_band, row, amp_buf);
    get_band_float_line(fin, self->meta, self->hh_phase_band, row, phase_buf);
    for (k=0; k<ns; ++k)
      self->lines[last][k].hh = complex_new_polar(sqrt(amp_buf[k]),
                                                  phase_buf[k]);
    
    get_band_float_line(fin, self->meta, self->hv_amp_band, row, amp_buf);
    get_band_float_line(fin, self->meta, self->hv_phase_band, row, phase_buf);
    for (k=0; k<ns; ++k)
      self->lines[last][k].hv = complex_new_polar(sqrt(amp_buf[k]),
                                                  phase_buf[k]);
    
    get_band_float_line(fin, self->meta, self->vh_amp_band, row, amp_buf);
    get_band_float_line(fin, self->meta, self->vh_phase_band, row, phase_buf);
    for (k=0; k<ns; ++k)
      self->lines[last][k].vh = complex_new_polar(sqrt(amp_buf[k]),
                                                  phase_buf[k]);
    
    get_band_float_line(fin, self->meta, self->vv_amp_band, row, amp_buf);
    get_band_float_line(fin, self->meta, self->vv_phase_band, row, phase_buf);
    for (k=0; k<ns; ++k)
      self->lines[last][k].vv = complex_new_polar(sqrt(amp_buf[k]),
                                                  phase_buf[k]);
    
    calculate_pauli_for_row(self, last);
    calculate_coherence_for_row(self, last);
  }
  else {
    // window has scrolled off top of image -- fill with zeros
    for (k=0; k<self->meta->general->sample_count; ++k) {
      self->lines[last][k] = qual_pol_zero();
      self->pauli_lines[last][k] = complex_vector_zero();
    }
  }
  
  free(amp_buf);
  free(phase_buf);
}

static void polarimetric_image_rows_load_new_rows(PolarimetricImageRows *self,
                                                  FILE *fin)
{
  int i,k,ns = self->meta->general->sample_count;
  float *amp_buf = MALLOC(sizeof(float)*ns);
  float *phase_buf = MALLOC(sizeof(float)*ns);

  int amp_band = self->amp_band;
  if (amp_band < 0)
    amp_band = self->hh_amp_band;

  // multilook the amplitude values as we go
  for (k=0; k<ns; ++k)
    self->amp[k] = 0.0;

  for (i=0; i<self->nrows; ++i) {
    int row = self->current_row + i;
    get_band_float_line(fin, self->meta, amp_band, row, amp_buf);
    for (k=0; k<ns; ++k)
      self->amp[k] += amp_buf[k];

    // now the SLC rows
    get_band_float_line(fin, self->meta, self->hh_amp_band, row, amp_buf);
    get_band_float_line(fin, self->meta, self->hh_phase_band, row, phase_buf);
    for (k=0; k<ns; ++k)
      self->lines[i][k].hh = complex_new_polar(sqrt(amp_buf[k]),
                                               phase_buf[k]);
    
    get_band_float_line(fin, self->meta, self->hv_amp_band, row, amp_buf);
    get_band_float_line(fin, self->meta, self->hv_phase_band, row, phase_buf);
    for (k=0; k<ns; ++k)
      self->lines[i][k].hv = complex_new_polar(sqrt(amp_buf[k]),
                                               phase_buf[k]);
    
    get_band_float_line(fin, self->meta, self->vh_amp_band, row, amp_buf);
    get_band_float_line(fin, self->meta, self->vh_phase_band, row, phase_buf);
    for (k=0; k<ns; ++k)
      self->lines[i][k].vh = complex_new_polar(sqrt(amp_buf[k]),
                                               phase_buf[k]);
    
    get_band_float_line(fin, self->meta, self->vv_amp_band, row, amp_buf);
    get_band_float_line(fin, self->meta, self->vv_phase_band, row, phase_buf);
    for (k=0; k<ns; ++k)
      self->lines[i][k].vv = complex_new_polar(sqrt(amp_buf[k]),
                                               phase_buf[k]);

    calculate_pauli_for_row(self, i);
    calculate_coherence_for_row(self, i);
  }

  // we multilook the amplitude data now, since we only keep one row
  // around-- the rest of the stuff is not multilooked since it isn't
  // the final result (all rows are kept in memory)
  for (k=0; k<ns; ++k)
    self->amp[k] /= self->nrows;

  self->current_row += self->nrows;

  free(amp_buf);
  free(phase_buf);
}

static void polarimetric_image_rows_free(PolarimetricImageRows* self)
{
    free(self->amp);
    free(self->data_buffer);
    free(self->lines);
    free(self->pauli_buffer);
    free(self->pauli_lines);

    int i;
    for (i=0; i<self->nrows*self->meta->general->sample_count; ++i)
        complex_matrix_free(self->coh_buffer[i]);

    free(self->coh_buffer);
    free(self->coh_lines);

    // do not free metadata pointer!
    free(self);
}

static double log3(double v)
{
    return log(v)/log(3.);
}

// Dump a 2D histogram image in entropy-alpha space
// the array is twice as wide as it is tall -- we use a square histogram,
// but generate two of them -- the "left" one is the 0<anisotropy<.5
// histogram, and the "right" is the .5<anisotropy<1 one.

// For Cloude-Pottier-8, anisotropy is ignored so when the histogram is
// dumped the two sides are added together, and we only dump the left
// half of the arrays when generating the histogram and classification map

// For Cloude-Potter-16, we dump the double-wide histograms and
// classification maps.

#define HIST_HEIGHT 512
static const int hist_height = HIST_HEIGHT;   // height
static const int hist_width = HIST_HEIGHT*2;   // width
int ea_hist[HIST_HEIGHT][HIST_HEIGHT*2];

static void dump_ea_hist(const char *base_filename,
                         int ea_hist[hist_height][hist_width],
                         int wide)
{
  char *filename = appendToBasename(base_filename, "_ea_hist");
  int size=hist_height;

  meta_parameters *m = raw_init();
  m->general->line_count = size;
  m->general->sample_count = wide ? size*2 : size;
  m->general->data_type = INTEGER16;
  strcpy(m->general->basename, filename);

  char *meta_file = appendExt(filename, ".meta");
  char *img_file = appendExt(filename, ".img");
  meta_write(m, meta_file);

  FILE *fp = fopenImage(img_file, "wb");

  int i,j;
  float *buf;

  if (wide) {
    buf = MALLOC(sizeof(float)*size*2);
    for (i=0; i<size; ++i) {
      for (j=0; j<size*2; ++j)
        buf[j] = ea_hist[i][j];
      put_float_line(fp,m,i,buf);
    }
  }
  else {
    buf = MALLOC(sizeof(float)*size);
    for (i=0; i<size; ++i) {
      for (j=0; j<size; ++j)
        buf[j] = ea_hist[i][j] + ea_hist[i][j+hist_height];
      put_float_line(fp,m,i,buf);
    }
  }

  free(buf);
  fclose(fp);
  meta_free(m);

  free(meta_file);
  free(img_file);
  free(filename);
}

static void dump_class_map(const char *base_filename,
                           int class_map[hist_height][hist_width],
                           int wide)
{
  char *filename = appendToBasename(base_filename, "_class_map");

  int height = hist_height;
  int width = wide ? hist_height*2 : hist_height;

  meta_parameters *m = raw_init();
  m->general->line_count = height;
  m->general->sample_count = width;
  m->general->data_type = BYTE;
  strcpy(m->general->basename, filename);

  char *meta_file = appendExt(filename, ".meta");
  char *img_file = appendExt(filename, ".img");
  meta_write(m, meta_file);

  FILE *fp = fopenImage(img_file, "wb");

  int i,j;
  float *buf = MALLOC(sizeof(float)*width);

  for (i=0; i<height; ++i) {
    for (j=0; j<width; ++j)
      buf[j] = class_map[i][j];
    put_float_line(fp,m,i,buf);
  }

  free(buf);
  fclose(fp);
  meta_free(m);

  free(meta_file);
  free(img_file);
  free(filename);
}

static double calc_alpha_real(double e)
{
  // still not entirely clear why we have to put fabs() here
  double alpha = acos(fabs(e));

  // alpha should be 0-90
  if (alpha < 0 || alpha > 1.571) {
    printf("Invalid alpha value: %f\n", alpha);
    alpha = 0;
  }

  return alpha;
}

static double calc_alpha(gsl_complex z)
{
  // alpha: acos(e[0]), e=eigenvector of coherence matrix
  return calc_alpha_real(GSL_REAL(z));
}

static void add_boundary(int ea_hist[hist_height][hist_width], int wide)
{
  const char *boundary_file = "classifications/ea_boundary.txt";

  if (share_file_exists(boundary_file)) {
    FILE *fp = fopen_share_file(boundary_file,"r");

    // fopen_share_file() dies if it failed to open
    assert(fp); 

    int k;
    char line[256];

    // skip past the header line, size line, blank line, headings line
    for (k=0; k<4; ++k)
      fgets(line,256,fp); 

    // now read the actual points
    while (fgets(line,256,fp)) {
      double entropy, alpha;
      sscanf(line,"%lf,%lf", &entropy, &alpha);
      int i = (hist_height-1)*(1-alpha/90.);
      int j = entropy*hist_height;
      ea_hist[i][j] = 255;
      if (wide)
        ea_hist[i][j+hist_height] = 255;
    }
    FCLOSE(fp);
  }
  else {
    asfPrintStatus("Boundary map file (ea_boundary.txt) file not found."
        "\n*** To get the entropy/alpha feasibilty region boundary on the "
        "classification\n    map, see asf_calpol.\n");
  }
}

static void do_sinclair_bands(int band1, int band2, int band3,
                              PolarimetricImageRows *img_rows,
                              int line, int l, int multi, int chunk_size,
                              meta_parameters *outMeta, FILE *fout,
                              float *buf)
{
  int j, m;
  int ns = outMeta->general->sample_count;

  if (multi) {
    // multilook case -- average all buffered lines to produce a
    // single output line
    if (band1 >= 0) {
      for (j=0; j<ns; ++j) {
        buf[j] = 0.0;
        for (m=0; m<chunk_size; ++m)
          buf[j] += complex_amp(img_rows->lines[m][j].hh);
        buf[j] /= (float)chunk_size;
      }
      put_band_float_line(fout, outMeta, band1, line, buf);
    }
    if (band2 >= 0) {
      for (j=0; j<ns; ++j) {
        buf[j] = 0.0;
        for (m=0; m<chunk_size; ++m) {
          complexFloat c = complex_add(img_rows->lines[m][j].hv,
                                       img_rows->lines[m][j].vh);
          buf[j] += complex_amp(complex_scale(c, 0.5));
        }
        buf[j] /= (float)chunk_size;
      }
      put_band_float_line(fout, outMeta, band2, line, buf);
    }
    if (band3 >= 0) {
      for (j=0; j<ns; ++j) {
        buf[j] = 0.0;
        for (m=0; m<chunk_size; ++m)
          buf[j] += complex_amp(img_rows->lines[m][j].vv);
        buf[j] /= (float)chunk_size;
      }
      put_band_float_line(fout, outMeta, band3, line, buf);
    }
  }
  else {
    // not multilooking -- no averaging necessary
    if (band1 >= 0) {
      for (j=0; j<ns; ++j)
        buf[j] = complex_amp(img_rows->lines[l][j].hh);
      put_band_float_line(fout, outMeta, band1, line, buf);
    }
    if (band2 >= 0) {
      for (j=0; j<ns; ++j) {
        complexFloat c = complex_add(img_rows->lines[l][j].hv,
                                     img_rows->lines[l][j].vh);
        buf[j] = complex_amp(complex_scale(c, 0.5));
      }
      put_band_float_line(fout, outMeta, band2, line, buf);
    }
    if (band3 >= 0) {
      for (j=0; j<ns; ++j)
        buf[j] = complex_amp(img_rows->lines[l][j].vv);
      put_band_float_line(fout, outMeta, band3, line, buf);
    }
  }
}

static void do_pauli_bands(int band1, int band2, int band3,
                           PolarimetricImageRows *img_rows,
                           int line, int l, int multi, int chunk_size,
                           meta_parameters *outMeta, FILE *fout,
                           float *buf)
{
  int j, m;
  int ns = outMeta->general->sample_count;

  if (multi) {
    // multilook case -- average all buffered lines to produce a
    // single output line
    if (band1 >= 0) {
      for (j=0; j<ns; ++j) {
        buf[j] = 0.0;
        for (m=0; m<chunk_size; ++m)
          buf[j] += complex_amp(img_rows->pauli_lines[m][j].A);
        buf[j] /= (float)chunk_size;
      }
      put_band_float_line(fout, outMeta, band1, line, buf);
    }
    if (band2 >= 0) {
      for (j=0; j<ns; ++j) {
        buf[j] = 0.0;
        for (m=0; m<chunk_size; ++m)
          buf[j] += complex_amp(img_rows->pauli_lines[m][j].B);
        buf[j] /= (float)chunk_size;
      }
      put_band_float_line(fout, outMeta, band2, line, buf);
    }
    if (band3 >= 0) {
      for (j=0; j<ns; ++j) {
        buf[j] = 0.0;
        for (m=0; m<chunk_size; ++m)
          buf[j] += complex_amp(img_rows->pauli_lines[m][j].C);
        buf[j] /= (float)chunk_size;
      }
      put_band_float_line(fout, outMeta, band3, line, buf);
    }
  }
  else {
    // not multilooking -- no averaging necessary
    if (band1 >= 0) {
      for (j=0; j<ns; ++j)
        buf[j] = complex_amp(img_rows->pauli_lines[l][j].A);
      put_band_float_line(fout, outMeta, band1, line, buf);
    }
    if (band2 >= 0) {
      for (j=0; j<ns; ++j)
        buf[j] = complex_amp(img_rows->pauli_lines[l][j].B);
      put_band_float_line(fout, outMeta, band2, line, buf);
    }
    if (band3 >= 0) {
      for (j=0; j<ns; ++j)
        buf[j] = complex_amp(img_rows->pauli_lines[l][j].C);
      put_band_float_line(fout, outMeta, band3, line, buf);
    }
  }
}

static void
do_coherence_bands(int entropy_band, int anisotropy_band, int alpha_band,
                   int class_band,
                   PolarimetricImageRows *img_rows,
                   int line, int l, int multi, int chunk_size,
                   gsl_matrix_complex *T, gsl_vector *eval,
                   gsl_matrix_complex *evec, gsl_eigen_hermv_workspace *ws,
                   meta_parameters *outMeta, FILE *fout,
                   float *buf, classifier_t *classifier)
{
  if (entropy_band >= 0 || anisotropy_band >= 0 || alpha_band >= 0 || 
      class_band >= 0)
  {
    int ns = outMeta->general->sample_count;
    int onl = outMeta->general->line_count;

    float *entropy = MALLOC(sizeof(float)*ns);
    float *anisotropy = MALLOC(sizeof(float)*ns);
    float *alpha = MALLOC(sizeof(float)*ns);

    // size of the horizontal window, used for ensemble averaging
    // actual window size is hw*2+1
    int hw;
    if (multi)
      hw = 0; // no horizontal averaging
    else
      hw = 2; // 5 pixels averaging horizontally

    // coherence -- do ensemble averaging for each element
    int j;
    for (j=0; j<ns; ++j) {
      int ii,jj,m;
      for (ii=0; ii<3; ++ii) {
        for (jj=0; jj<3; ++jj) {
          gsl_complex c = gsl_complex_rect(0,0);
          int k,n=0;
          for (m=0; m<chunk_size; ++m) {
            for (k=j-hw;k<=j+hw;++k) {
              if (k>=0 && k<ns && m+line>l && m+line<onl-l) {
                ++n;
                complexFloat f =
                  img_rows->coh_lines[m][k]->coeff[ii][jj];
                c = gsl_complex_add_real(c, f.real);
                c = gsl_complex_add_imag(c, f.imag);
              }
            }
          }
          if (n>1) {
            gsl_complex_div_real(c, (float)n);
          }
          gsl_matrix_complex_set(T,ii,jj,c);
        }
      }
      
      gsl_eigen_hermv(T, eval, evec, ws);
      gsl_eigen_hermv_sort(eval, evec, GSL_EIGEN_SORT_ABS_DESC);
      
      double e1 = gsl_vector_get(eval, 0);
      double e2 = gsl_vector_get(eval, 1);
      double e3 = gsl_vector_get(eval, 2);
      
      double eT = e1+e2+e3;
      
      double P1 = e1/eT;
      double P2 = e2/eT;
      double P3 = e3/eT;
      
      double P1l3 = log3(P1);
      double P2l3 = log3(P2);
      double P3l3 = log3(P3);
      
      // If a Pn value is small enough, the log value will be NaN.
      // In this case, the value of -Pn*log3(Pn) is supposed to be
      // zero - we have to force it.
      entropy[j] =
        (meta_is_valid_double(P1l3) ? -P1*P1l3 : 0) +
        (meta_is_valid_double(P2l3) ? -P2*P2l3 : 0) +
        (meta_is_valid_double(P3l3) ? -P3*P3l3 : 0);
      
      // mathematically, entropy is limited to be between 0 and 1.
      // however it sometimes is just a bit out of that range due
      // to numerical anomalies
      if (!meta_is_valid_double(entropy[j]))
        entropy[j] = 0.0;
      else if (entropy[j] < 0)
        entropy[j] = 0.0;
      else if (entropy[j] > 1)
        entropy[j] = 1.0;
      
      if (e2+e3 != 0)
        anisotropy[j] = (e2-e3)/(e2+e3);
      else
        anisotropy[j] = 0;
      
      // as for entropy, anisotropy is limited to be between 0 and 1.
      // guard against numerical anomalies (usually this is due to
      // one really big eigenvalue)
      if (!meta_is_valid_double(anisotropy[j]))
        anisotropy[j] = 0.0;
      else if (anisotropy[j] < 0)
        anisotropy[j] = 0.0;
      else if (anisotropy[j] > 1)
        anisotropy[j] = 1.0;
      
      // calculate the "mean alpha" (mean scattering angle)
      // this is the polar angle when expressing each eigenvector
      // in spherical coordinates.  the mean alpha is weighted by
      // the eigenvector (so weight by P1-3)
      double alpha1 = calc_alpha(gsl_matrix_complex_get(evec, 0, 0));
      double alpha2 = calc_alpha(gsl_matrix_complex_get(evec, 0, 1));
      double alpha3 = calc_alpha(gsl_matrix_complex_get(evec, 0, 2));
      
      alpha[j] = R2D*(P1*alpha1 + P2*alpha2 + P3*alpha3);
      if (!meta_is_valid_double(alpha[j]))
        alpha[j] = 0.0;
    }
    
    if (entropy_band >= 0)
      put_band_float_line(fout, outMeta, entropy_band, line, entropy);
    if (anisotropy_band >= 0)
      put_band_float_line(fout, outMeta, anisotropy_band, line, anisotropy);
    if (alpha_band >= 0)
      put_band_float_line(fout, outMeta, alpha_band, line, alpha);
    
    if (class_band >= 0) {
      assert(classifier != NULL);
      for (j=0; j<ns; ++j) {
        buf[j] = (float)classify(classifier, entropy[j], anisotropy[j],
                                 alpha[j]);
      }
      put_band_float_line(fout, outMeta, class_band, line, buf);
    }
    
    for (j=0; j<ns; ++j) {
      int entropy_index = entropy[j]*(float)hist_height;
      if (entropy_index<0) entropy_index=0;
      if (entropy_index>hist_height-1) entropy_index=hist_height-1;
            
      int alpha_index = hist_height-1-alpha[j]/90.0*(float)hist_height;
      if (alpha_index<0) alpha_index=0;
      if (alpha_index>hist_height-1) alpha_index=hist_height-1;
      
      //printf("%10.1f %10.1f %5d %5d --> %4d\n",
      //       entropy[j], alpha[j],
      //       entropy_index, alpha_index,
      //      ea_hist[entropy_index][alpha_index]+1);
      if (anisotropy[j] > .5)
        entropy_index += hist_height;
      ea_hist[alpha_index][entropy_index] += 1;
    }

    free(entropy);
    free(anisotropy);
    free(alpha);
  }
}

static int verify_equal_re(const char *id, float lhs, float rhs)
{
  if (fabs(lhs - rhs) > .001) {
    //printf("Not equal (%s): %f != %f\n", id, lhs, rhs);
    return FALSE;
  }
  return TRUE;
}

static int verify_equal_cpx(const char *id, complexFloat lhs, complexFloat rhs)
{
  if (fabs(lhs.real - rhs.real) > .001 ||
      fabs(lhs.imag - rhs.imag) > .001)
  {
    //printf("Not equal (%s): (%f,%f) != (%f,%f)\n", id,
    //       lhs.real, lhs.imag, rhs.real, rhs.imag);
    return FALSE;
  }
  return TRUE;
}

static int verify_fd(float hh2, float vv2, complexFloat hhvv, float fs,
                     float fd, complexFloat alpha, complexFloat beta)
{
  int ok1,ok2,ok3;

  // (1) hh2 = fs*beta*beta + fd*alpha*alpha
  ok1 = verify_equal_re("Eq. 1", hh2, fs*complex_amp_sqr(beta) +
                                      fd*complex_amp_sqr(alpha));
  // (2) vv2 = fs + fd
  ok2 = verify_equal_re("Eq. 2", vv2, fs+fd);

  // (3) hhvv = fs*beta + fd*alpha
  ok3 = verify_equal_cpx("Eq. 3", hhvv, complex_add(
                           complex_scale(beta, fs), complex_scale(alpha, fd)));

  return ok1 && ok2 && ok3;
}

static void solve_fd1(float hh2, float vv2, complexFloat hhvv,
                      float *fs, float *fd, complexFloat *beta)
{
  // 3 equations with 3 unknowns: (complex)
  //   |hh|^2 = fd + |b|^2 * fs
  //   |vv|^2 = fs + fd
  //   hh*conj(vv) = b*fs - fd
  
  // fs, fd are real.  beta (b) is complex.
  // splitting beta into br and bi (real and imaginary)
  // 4 equations and 4 unknown: (real)
  //   |hh|^2 = fd + |b|^2 * fs
  //   |vv|^2 = fs + fd
  //   Re(hh*conj(vv)) = br*fs - fd
  //   Im(hh*conj(vv)) = bi*fs

  // For brevity:
  float x = hhvv.real;
  float y = hhvv.imag;
  float h = hh2;
  float v = vv2;

  // Analytic solution of the 4 equations & 4 unknowns:
  //   fd = (-x^2 - y^2 + hv) / D1
  //   br = (hx + v^2 + hv + 3xv + 2x^2) / D2 - 1
  //   bi = D1 * y / D2
  //   fs = D2 / D1
  // where
  //   D1 = h + v + 2x
  //   D2 = v^2 + 2xv + x^2 + y^2
  
  float D1 = h + v + 2.*x;
  float D2 = v*v + 2.*x*v + x*x + y*y;
  
  *fd = (h*v - x*x - y*y) / D1;
  *fs = D2 / D1;
     
  float br = (h*x + v*v + h*v + 3.*x*v + 2.*x*x) / D2 - 1.;
  float bi = D1 * y / D2;
     
  *beta = complex_new(br,bi);
}

static void solve_fd2(float hh2, float vv2, complexFloat hhvv,
                      float *fs, float *fd, complexFloat *alpha)
{
  // 3 equations with 3 unknowns: (complex)
  //   |hh|^2 = fs + |a|^2 * fd
  //   |vv|^2 = fs + fd
  //   hh*conj(vv) = a*fd + fs
  
  // fs, fd are real.  alpha (a) is complex.
  // splitting alpha into ar and ai (real and imaginary)
  // 4 equations and 4 unknown: (real)
  //   |hh|^2 = fs + |a|^2 * fd
  //   |vv|^2 = fs + fd
  //   Re(hh*conj(vv)) = ar*fd + fs
  //   Im(hh*conj(vv)) = ai*fd

  // For brevity:
  float x = hhvv.real;
  float y = hhvv.imag;
  float h = hh2;
  float v = vv2;

  // Analytic solution of the 4 equations & 4 unknowns:
  //   fs = (-x^2 - y^2 + hv) / D1
  //   ar = (hx - v^2 - hv + 3xv - 2x^2) / D2 + 1
  //   ai = D1 * y / D2
  //   fd = D2 / D1
  // where
  //   D1 = h + v - 2x
  //   D2 = v^2 - 2xv + x^2 + y^2
  
  float D1 = h + v - 2.*x;
  float D2 = v*v - 2.*x*v + x*x + y*y;
  
  *fs = (h*v - x*x - y*y) / D1;
  *fd = D2 / D1;
     
  float ar = (h*x - v*v - h*v + 3.*x*v - 2.*x*x) / D2 + 1.;
  float ai = D1 * y / D2;
     
  *alpha = complex_new(ar,ai);
}

static void do_freeman(int band1, int band2, int band3,
                       PolarimetricImageRows *img_rows,
                       int line, int l, int multi, int chunk_size,
                       meta_parameters *outMeta, FILE *fout)
{
  if (band1 >= 0 || band2 >= 0 || band3 >= 0)
  {
    //if (outMeta->general->radiometry != r_SIGMA) {
    //  asfPrintError("The Freeman/Durden decomposition requires "
    //                "PowerScale (Sigma, non-db) data.\n");
    //}

    int j, m;
    int ns = outMeta->general->sample_count;

    float *hh2 = MALLOC(sizeof(float)*ns);
    float *vv2 = MALLOC(sizeof(float)*ns);
    float *hv2 = MALLOC(sizeof(float)*ns);
    complexFloat *hhvv = MALLOC(sizeof(complexFloat)*ns);
    float sf = 1.0 / (float)chunk_size;

    if (multi) {
      // multilook case -- average all buffered lines to produce a
      // single output line
      for (j=0; j<ns; ++j) {

        hh2[j] = 0.0;
        vv2[j] = 0.0;
        hhvv[j] = complex_zero();
        hv2[j] = 0.0;

        for (m=0; m<chunk_size; ++m) {
          complexFloat hh = img_rows->lines[m][j].hh;
          hh2[j] += complex_amp_sqr(hh);

          complexFloat vv = img_rows->lines[m][j].vv;
          vv2[j] += complex_amp_sqr(vv);

          hhvv[j] = complex_add(hhvv[j], complex_mul(hh, complex_conj(vv)));

          hv2[j] += complex_amp_sqr(img_rows->lines[m][j].hv);
        }

        hh2[j] *= sf;
        vv2[j] *= sf;
        hv2[j] *= sf;

        hhvv[j] = complex_scale(hhvv[j], sf);
      }
    }
    else {
      // not multilooking -- no averaging necessary
      for (j=0; j<ns; ++j) {
        complexFloat hh = img_rows->lines[l][j].hh;
        hh2[j] = complex_amp_sqr(hh);

        complexFloat vv = img_rows->lines[l][j].vv;
        vv2[j] = complex_amp_sqr(vv);

        hhvv[j] = complex_mul(hh, complex_conj(vv));
        hv2[j] = complex_amp_sqr(img_rows->lines[l][j].hv);
      }
    }

    float *Ps = MALLOC(sizeof(float)*ns);
    float *Pd = MALLOC(sizeof(float)*ns);
    float *Pv = MALLOC(sizeof(float)*ns);

    // now calculate fs, fd and alpha or beta for each sample, and
    // from those we can get the Ps, Pd, and Pv values
    for (j=0; j<ns; ++j) {
      float fs, fd;
      complexFloat alpha, beta;
      if (hhvv[j].real > 0) {
        // Re(Shh*conj(Svv))>0 ==> alpha=-1, solve for fs, fd, and beta
        solve_fd1(hh2[j], vv2[j], hhvv[j], &fs, &fd, &beta);
        alpha = complex_new(-1, 0);
      }
      else {
        // Re(Shh*conj(Svv))<0 ==> beta=1, solve for fs, fd, and alpha
        solve_fd2(hh2[j], vv2[j], hhvv[j], &fs, &fd, &alpha);
        beta = complex_new(1, 0);
      }

      // double-check the solution
      verify_fd(hh2[j], vv2[j], hhvv[j], fs, fd, alpha, beta);

      // now calculate the final contributions from each scattering mechanism
      Ps[j] = fs * (1. + complex_amp_sqr(beta));
      Pd[j] = fd * (1. + complex_amp_sqr(alpha));
      Pv[j] = 8. * hv2[j];

      // convert to dB
      Ps[j] = 10*log10(Ps[j]*Ps[j]);
      Pd[j] = 10*log10(Pd[j]*Pd[j]);
      Pv[j] = 10*log10(Pv[j]*Pv[j]);
    }

    free(hh2);
    free(vv2);
    free(hv2);
    free(hhvv);

    if (band1 >= 0)
      put_band_float_line(fout, outMeta, band1, line, Ps);
    if (band2 >= 0)
      put_band_float_line(fout, outMeta, band2, line, Pd);
    if (band3 >= 0)
      put_band_float_line(fout, outMeta, band3, line, Pv);

    free(Pd);
    free(Ps);
    free(Pv);
  }
}

static void do_class_map(classifier_t *classifier, int class_band, int wide,
                         const char *outFile)
{
  // dump classified pop graph -- reuse the same ea_hist array,
  // since we are done with the population graph
  int i, j;
  if (class_band >= 0) {
    if (!wide) {
      // non-wide: this is the Cloude-Pottier 8 classes case, where
      // we use only half of the ea_hist array
      for (i=0; i<hist_height; ++i) {
        double alpha = (double)(hist_height-1-i)/(double)hist_height*90.;
        for (j=0; j<hist_height; ++j) {
          int count = ea_hist[i][j] + ea_hist[i][j+hist_height];
          if (j==0) {
            ea_hist[i][j] = 0;
          }
          else if (count > 0) {
            double entropy = (double)j/(double)hist_height;
            ea_hist[i][j] = classify(classifier, entropy, 0, alpha);
          }
        }
      }
      // drawing the white border lines -- assumes look-up-tables use
      // "255" as white.  (cloude8.lut)
      int prev = -1;
      for (i=0; i<hist_height; ++i) {
        double alpha = (double)(hist_height-1-i)/(double)hist_height*90.;
        for (j=0; j<hist_height; ++j) {
          double entropy = (double)j/(double)hist_height;
          int curr = classify(classifier, entropy, 0, alpha);
          if (j>0 && prev != curr)
            ea_hist[i][j] = 255;
          prev = curr;
        }
      }
      prev = -1;
      for (j=0; j<hist_height; ++j) {
        double entropy = (double)j/(double)hist_height;
        for (i=0; i<hist_height; ++i) {
          double alpha = (double)(hist_height-1-i)/(double)hist_height*90.;
          int curr = classify(classifier, entropy, 0, alpha);
          if (i>0 && prev != curr)
            ea_hist[i][j] = 255;
          prev = curr;
        }
      }
      // if we have the file "ea_boundary.txt" in the share directory
      // (this file can be generated by calling make_entropy_alpha_boundary()
      // defined below, and exposed via asf_calpol)
      add_boundary(ea_hist, FALSE);

      dump_class_map(outFile, ea_hist, FALSE);
    }
    else {
      // wide: this is the Cloude-Pottier 16 classes case, where
      // we use the full (double-width) ea_hist array, the left half
      // being the anisotropy=0 plane, the right half being the
      // anisotropy=.5 plane (though we actually use .55 below to ensure
      // we're definitely in the >.5 blocks -- .5 could possibly be
      // classified in the (0,.5) range depending on use of <= vs < above,
      // roundoff, etc)
      for (i=0; i<hist_height; ++i) {
        double alpha = (double)(hist_height-1-i)/(double)hist_height*90.;
        for (j=0; j<hist_height*2; ++j) {
          int count = ea_hist[i][j];
          if (j==0) {
            ea_hist[i][j] = 0;
          }
          else if (j == hist_height) {
            ea_hist[i][j] = 255;
          }
          else if (count > 0) {
            double entropy = j > hist_height ?
              (double)(j-hist_height)/(double)hist_height :
              (double)j/(double)hist_height;
            double aniso = j > hist_height ? 0.55 : 0.0;
            ea_hist[i][j] = classify(classifier, entropy, aniso, alpha);
          }
        }
      }
      // drawing the white border lines -- assumes look-up-tables use
      // "255" as white.  (cloude16.lut)
      int prev = -1;
      for (i=0; i<hist_height; ++i) {
        double alpha = (double)(hist_height-1-i)/(double)hist_height*90.;
        for (j=0; j<hist_height*2; ++j) {
          double entropy = j > hist_height ?
            (double)(j-hist_height)/(double)hist_height :
            (double)j/(double)hist_height;
          double aniso = j > hist_height ? 0.55 : 0.0;
          int curr = classify(classifier, entropy, aniso, alpha);
          if (j>0 && prev != curr)
            ea_hist[i][j] = 255;
          prev = curr;
        }
      }
      prev = -1;
      for (j=0; j<hist_height*2; ++j) {
        double entropy = j > hist_height ?
          (double)(j-hist_height)/(double)hist_height :
          (double)j/(double)hist_height;
        double aniso = j > hist_height ? 0.55 : 0.0;
        for (i=0; i<hist_height; ++i) {
          double alpha = (double)(hist_height-1-i)/(double)hist_height*90.;
          int curr = classify(classifier, entropy, aniso, alpha);
          if (i>0 && prev != curr)
            ea_hist[i][j] = 255;
          prev = curr;
        }
      }
      // if we have the file "ea_boundary.txt" in the share directory
      // (this file can be generated by calling make_entropy_alpha_boundary()
      // defined below, and exposed via asf_calpol)
      add_boundary(ea_hist, TRUE);
      
      dump_class_map(outFile, ea_hist, TRUE);
    }
  }
}

void polarimetric_decomp(const char *inFile, const char *outFile,
                         int amplitude_band,
                         int pauli_1_band,
                         int pauli_2_band,
                         int pauli_3_band,
                         int entropy_band,
                         int anisotropy_band,
                         int alpha_band,
                         int sinclair_1_band,
                         int sinclair_2_band,
                         int sinclair_3_band,
                         int freeman_1_band,
                         int freeman_2_band,
                         int freeman_3_band,
                         const char *classFile,
                         int class_band)
{
  int debug_mode = classFile && strcmp_case(classFile, "DEBUG")==0;
  int debug_band = -1;
  if (debug_mode) {
    debug_band = class_band;
    class_band = -1;
  }

  char *meta_name = appendExt(inFile, ".meta");
  meta_parameters *inMeta = meta_read(meta_name);
  meta_parameters *outMeta = meta_read(meta_name);

  char *in_img_name = appendExt(inFile, ".img");
  char *out_img_name = appendExt(outFile, ".img");

  int i, j;

  // chunk_size represents the number of rows we keep in memory at one
  // time, centered on the row currently being processed.  This is to
  // handle the ensemble averaging that we do
  int chunk_size = 5;
  assert((chunk_size-1)%2==0); // chunk_size should be odd

  // If the image is not multilooked, we will multilook it here, and use
  // the multilooked values for the ensemble averaging.  This will result
  // in much less smoothing of the image.
  int multi = FALSE;
  if (inMeta->sar && inMeta->sar->multilook==0) {
    multi = TRUE;
    chunk_size = inMeta->sar->look_count;
  }

  // aliases
  int nl = inMeta->general->line_count;
  int ns = inMeta->general->sample_count;

  FILE *fin = fopenImage(in_img_name, "rb");
  FILE *fout = fopenImage(out_img_name, "wb");

  // this struct will hold the current row being processed, and
  // chunk_size/2 rows before & after
  PolarimetricImageRows *img_rows =
      polarimetric_image_rows_new(inMeta, chunk_size, multi);

  // make sure all bands we need are there, and find their numbers
  // and offsets
  int ok = polarimetric_image_rows_get_bands(img_rows);

  if (!ok)
      asfPrintError("Not all required bands found-- "
                    "is this SLC quad-pol data?\n");

  float *buf = MALLOC(sizeof(float)*ns);

  // at the start, we want to load the buffers as follows: (for chunk_size=5)
  //   *lines[0] = ALL ZEROS
  //   *lines[1] = ALL ZEROS
  //   *lines[2] = line 0 of the image
  //   *lines[3] = line 1 of the image
  //   *lines[4] = line 2 of the image
  // next time through the loop:
  //   *lines[0] = ALL ZEROS
  //   *lines[1] = line 0 of the image
  //   *lines[2] = line 1 of the image
  //   *lines[3] = line 2 of the image
  //   *lines[4] = line 3 of the image
  // we don't actually move the data from line n to line n-1, we just move
  // the pointers.  initially, the pointers will match the buffer (as set
  // in the loop directly above), but the second time through the pointers
  // slide down one row (line 3 is loaded into the beginning of the buffer,
  // but line pointer 4 points at the beginning).

  if (multi) {
    // multilook case:
    // preload rows --> load look_count rows, these will all be combined
    // to produce a single output line
    polarimetric_image_rows_load_new_rows(img_rows, fin);
  }
  else {
    // non-multilook case:
    // preload rows --> center of window will be row 0.
    // the next (chunk_size+1)/2 rows are also loaded, and ready to go.
    for (i=0; i<(chunk_size+1)/2; ++i)
      polarimetric_image_rows_load_next_row(img_rows, fin);    
    assert(img_rows->current_row == 0);
  }

  // output metadata differs from input only in the number
  // of bands, and the band names
  char *out_meta_name = appendExt(outFile, ".meta");
  int nBands =
      (amplitude_band>=0) +
      (pauli_1_band>=0) + (pauli_2_band>=0) + (pauli_3_band>=0) +
      (entropy_band>=0) + (anisotropy_band>=0) + (alpha_band>=0) +
      (sinclair_1_band>=0) + (sinclair_2_band>=0) + (sinclair_3_band>=0) +
      (class_band >= 0) +
      (freeman_1_band>=0) + (freeman_2_band>=0) + (freeman_3_band>=0);

  char bands[255];
  strcpy(bands, "");

  for (i=0; i<10; ++i) {
      if (amplitude_band == i)
          strcat(bands, "HH-AMP,");
      else if (sinclair_1_band == i)
          strcat(bands, "HH,");
      else if (sinclair_2_band == i)
          strcat(bands, "HV+VH_2,");
      else if (sinclair_3_band == i)
          strcat(bands, "VV,");
      else if (pauli_1_band == i)
          strcat(bands, "HH-VV,");
      else if (pauli_2_band == i)
          strcat(bands, "HV+VH,");
      else if (pauli_3_band == i)
          strcat(bands, "HH+VV,");
      else if (entropy_band == i)
          strcat(bands, "Entropy,");
      else if (anisotropy_band == i)
          strcat(bands, "Anisotro,"); // abbreviated version, agrees with GUI
      else if (alpha_band == i)
          strcat(bands, "Alpha,");
      else if (freeman_1_band == i)
          strcat(bands, "Ps,");
      else if (freeman_2_band == i)
          strcat(bands, "Pd,");
      else if (freeman_3_band == i)
          strcat(bands, "Pv,");
      else if (class_band == i) {
        if (!classFile)
          strcat(bands,"Classified,");
        else if (strncmp_case(classFile,"cloude8",7)==0)
          strcat(bands, "Cloude-Pottier-8,");
        else if (strncmp_case(classFile,"cloude16",8)==0)
          strcat(bands, "Cloude-Pottier-16,");
        else {
          // append the classification filename as the band name (minus .cla)
          char *s = appendExt(classFile,"");
          strcat(bands, s);
          strcat(bands, ",");
          free(s);
        }
      }
      else
          break;
  }

  if (strlen(bands) > 0) // chop last comma
      bands[strlen(bands)-1] = '\0';

  outMeta->general->band_count = nBands;
  strcpy(outMeta->general->bands, bands);

  // for multilooking, the number of output lines shrinks by look_count
  int onl = multi ? nl/chunk_size : nl;

  if (multi) {
    outMeta->sar->multilook = 1;
    outMeta->general->line_count = onl;
    outMeta->general->y_pixel_size *= outMeta->sar->look_count;
    outMeta->sar->azimuth_time_per_pixel *= outMeta->sar->look_count;
  }

  meta_write(outMeta, out_meta_name);
  free(out_meta_name);

  // set up the classification, if needed
  classifier_t *classifier = NULL;
  if (classFile != NULL && class_band >= 0)
    classifier = read_classifier(classFile);

  // population histogram image, in entropy-alpha space
  for (i=0; i<hist_height; ++i)
    for (j=0; j<hist_width; ++j)
      ea_hist[i][j] = 0;

  //-----------------------------------------------------------------------
  // done setting up metadata, now write the data

  // gsl infrastructure for calculating eigen- vals & vecs for the
  // coherence matrix
  gsl_matrix_complex *T = gsl_matrix_complex_alloc(3,3);
  gsl_vector *eval = gsl_vector_alloc(3);
  gsl_matrix_complex *evec = gsl_matrix_complex_alloc(3,3);
  gsl_eigen_hermv_workspace *ws = gsl_eigen_hermv_alloc(3);

  // now loop through the lines of the output image
  for (i=0; i<onl; ++i) {

      // Indicates which line in the various *lines arrays contains
      // what corresponds to line i in the output. since the line pointers
      // slide, this never changes.
      const int l = (chunk_size-1)/2;

      // normal amplitude band (usually, this is added to allow terrcorr)
      if (amplitude_band >= 0)
        put_band_float_line(fout, outMeta, amplitude_band, i, img_rows->amp);

      // if requested, generate sinlair output
      do_sinclair_bands(sinclair_1_band, sinclair_2_band, sinclair_3_band,
                        img_rows, i, l, multi, chunk_size, outMeta, fout, buf);

      // calculate the pauli output (magnitude of already-calculated
      // complex pauli basis elements), and save the requested pauli
      // bands in the output
      do_pauli_bands(pauli_1_band, pauli_2_band, pauli_3_band,
                     img_rows, i, l, multi, chunk_size, outMeta, fout, buf);


      // Freeman-Durden
      do_freeman(freeman_1_band, freeman_2_band, freeman_3_band,
                 img_rows, i, l, multi, chunk_size, outMeta, fout);

      // do any polarimetry that uses the coherence matrix
      do_coherence_bands(entropy_band, anisotropy_band, alpha_band, class_band,
                         img_rows, i, l, multi, chunk_size, T, eval, evec, ws,
                         outMeta, fout, buf, classifier);
                         

      // load the next row, if there are still more to go
      if (i<onl-1) {
          if (multi) {
              polarimetric_image_rows_load_new_rows(img_rows, fin);
          }
          else {
              polarimetric_image_rows_load_next_row(img_rows, fin);
              assert(img_rows->current_row == i+1);
          }
      }

      asfLineMeter(i,onl);
  }

  if (entropy_band >= 0 || anisotropy_band >= 0 || alpha_band >= 0 || 
      class_band >= 0)
  {

    if (entropy_band >= 0 || anisotropy_band >= 0 || alpha_band >= 0)
      asfPrintStatus("Generating population histogram...\n");
    else
      asfPrintStatus("Generating population histogram and "
                     "classification map...\n");

    // dump population graph & class map
    int wide = FALSE;
    if (class_band >= 0)
      wide = strncmp_case(classFile,"cloude16",8) == 0;
    dump_ea_hist(outFile, ea_hist, wide);
    do_class_map(classifier, class_band, wide, outFile);
  }

  gsl_vector_free(eval);
  gsl_eigen_hermv_free(ws);
  gsl_matrix_complex_free(evec);
  gsl_matrix_complex_free(T);

  polarimetric_image_rows_free(img_rows);

  fclose(fin);
  fclose(fout);

  free(buf);

  free(out_img_name);
  free(in_img_name);
  free(meta_name);
  free_classifier(classifier);

  meta_free(inMeta);
  meta_free(outMeta);
}

static int has_amp_band(const char *inFile)
{
    int ret = FALSE;
    char *mfile = appendExt(inFile, ".meta");
    if (fileExists(mfile)) {
      meta_parameters *meta = meta_read(mfile);
      if (strncmp_case(meta->general->bands, "AMP", 3) == 0)
        ret = TRUE;
      meta_free(meta);
    }
    free(mfile);
    return ret;
}

void cpx2classification(const char *inFile, const char *outFile,
                        int tc_flag, const char *classFile)
{
  if (tc_flag && has_amp_band(inFile))
    polarimetric_decomp(inFile, outFile,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                        -1,-1,-1,classFile, 1);
  else
    polarimetric_decomp(inFile, outFile,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                        -1,-1,-1,classFile, 0);
}

void cpx2sinclair(const char *inFile, const char *outFile, int tc_flag)
{
  asfPrintStatus("\n\nGenerating Sinclair decomposition channels\n");
  if (tc_flag && has_amp_band(inFile))
    polarimetric_decomp(inFile,outFile,0,-1,-1,-1,-1,-1,-1,1,2,3,-1,-1,-1,
                        NULL,-1);
  else
    polarimetric_decomp(inFile,outFile,-1,-1,-1,-1,-1,-1,-1,0,1,2,-1,-1,-1,
                        NULL,-1);
}

void cpx2pauli(const char *inFile, const char *outFile, int tc_flag)
{
  asfPrintStatus("\n\nGenerating Pauli decomposition channels\n");
  if (tc_flag && has_amp_band(inFile))
    polarimetric_decomp(inFile,outFile,0,1,2,3,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                        NULL,-1);
  else
    polarimetric_decomp(inFile,outFile,-1,0,1,2,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                        NULL,-1);
}

void cpx2cloude_pottier(const char *inFile, const char *outFile, int tc_flag)
{
  asfPrintStatus("\n\nCalculating entropy, anisotropy and alpha "
		 "for Cloude-Pottier classification\n");
  cpx2classification(inFile, outFile, tc_flag, "cloude8.cla");
}

void cpx2cloude_pottier8(const char *inFile, const char *outFile, int tc_flag)
{
  asfPrintStatus("\n\nCalculating entropy, anisotropy and alpha "
		 "for Cloude-Pottier classification (8 classes)\n");
  cpx2classification(inFile, outFile, tc_flag, "cloude8.cla");
}

void cpx2cloude_pottier16(const char *inFile, const char *outFile, int tc_flag)
{
  asfPrintStatus("\n\nCalculating entropy, anisotropy and alpha "
		 "for Cloude-Pottier classification (16 classes)\n");
  cpx2classification(inFile, outFile, tc_flag, "cloude16.cla");
}

void cpx2entropy_anisotropy_alpha(const char *inFile, const char *outFile,
                                  int tc_flag)
{
  asfPrintStatus("\n\nCalculating entropy, anisotropy and alpha.\n");
  if (tc_flag && has_amp_band(inFile))
    polarimetric_decomp(inFile,outFile,0,-1,-1,-1,1,2,3,-1,-1,-1,-1,-1,-1,
                        NULL,-1);
  else 
    polarimetric_decomp(inFile,outFile,-1,-1,-1,-1,0,1,2,-1,-1,-1,-1,-1,-1,
                        NULL,-1);
}

void cpx2freeman_durden(const char *inFile, const char *outFile, int tc_flag)
{
  asfPrintStatus("\n\nGenerating Freeman/Durden decomposition channels\n");
  if (tc_flag && has_amp_band(inFile))
    polarimetric_decomp(inFile,outFile,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,1,2,3,
                        NULL,-1);
  else
    polarimetric_decomp(inFile,outFile,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,2,
                        NULL,-1);
}

static gsl_matrix *make_diag3(double e00, double e11, double e22)
{
  gsl_matrix *mat = gsl_matrix_calloc(3, 3);
  gsl_matrix_set(mat, 0, 0, e00);
  gsl_matrix_set(mat, 1, 1, e11);
  gsl_matrix_set(mat, 2, 2, e22);
  return mat;
}

static void calc_entropy_alpha(gsl_matrix *mat, double *entropy, double *alpha)
{
  gsl_vector *eval = gsl_vector_alloc(3);
  gsl_matrix *evec = gsl_matrix_alloc(3,3);

  gsl_eigen_symmv_workspace *w = gsl_eigen_symmv_alloc(3);
  gsl_eigen_symmv(mat, eval, evec, w);
  gsl_eigen_symmv_free(w);

  gsl_eigen_symmv_sort(eval, evec, GSL_EIGEN_SORT_ABS_ASC);

  double e1 = gsl_vector_get(eval, 0);
  double e2 = gsl_vector_get(eval, 1);
  double e3 = gsl_vector_get(eval, 2);

  double eT = e1+e2+e3;

  double P1 = e1/eT;
  double P2 = e2/eT;
  double P3 = e3/eT;

  double P1l3 = log3(P1);
  double P2l3 = log3(P2);
  double P3l3 = log3(P3);

  // If a Pn value is small enough, the log value will be NaN.
  // In this case, the value of -Pn*log3(Pn) is supposed to be
  // zero - we have to force it.
  *entropy =
    (meta_is_valid_double(P1l3) ? -P1*P1l3 : 0) +
    (meta_is_valid_double(P2l3) ? -P2*P2l3 : 0) +
    (meta_is_valid_double(P3l3) ? -P3*P3l3 : 0);

  // calculate the "mean alpha" (mean scattering angle)
  // this is the polar angle when expressing each eigenvector
  // in spherical coordinates.  the mean alpha is weighted by
  // the eigenvector (so weight by P1-3)
  double alpha1 = calc_alpha_real(gsl_matrix_get(evec, 0, 0));
  double alpha2 = calc_alpha_real(gsl_matrix_get(evec, 0, 1));
  double alpha3 = calc_alpha_real(gsl_matrix_get(evec, 0, 2));

  *alpha = R2D*(P1*alpha1 + P2*alpha2 + P3*alpha3);
  if (!meta_is_valid_double(*alpha))
    *alpha = 0.0;

  gsl_vector_free(eval);
  gsl_matrix_free(evec);
}

void make_entropy_alpha_boundary(const char *fname, int size)
{
  FILE *fp = FOPEN(fname, "w");

  fprintf(fp, "Entropy/Alpha Feasible Region Boundary\n");
  fprintf(fp, "File Size: %d\n\n", size);
  fprintf(fp, "Entropy,Alpha\n");

  // number of points generated in each curve's section
  int numtop = size/2;
  int numbot1 = size/4;
  int numbot2 = size/4;

  // fix any roundoff problems, while ensuring that numbot1 & numbot2
  // are equal
  while (numtop+numbot1+numbot2 < size) {
    ++numbot2; ++numbot1;
  }
  while (numtop+numbot1+numbot2 > size) {
    --numtop;
  }

  assert(numtop+numbot1+numbot2 == size);
 
  int i;
  double entropy, alpha;

  // Generate the top curve by finding the eigenvalues of
  // the matrix:
  //   [ 1 0 0 ]
  //   [ 0 m 0 ]
  //   [ 0 0 m ]
  // for 0 <= m <= 1

  for (i=0; i<numtop; ++i) {
    double m = (double)i / (double)(numtop-1);
    gsl_matrix *mat = make_diag3(1, m, m);
    calc_entropy_alpha(mat, &entropy, &alpha);
    fprintf(fp,"%f,%f\n",entropy,alpha);
    gsl_matrix_free(mat);
    asfPercentMeter((double)i/size);
  }

  // Generate the first part of the bottom curve by finding the eigenvalues
  // of the matrix:
  //  [ 1 0 0  ]
  //  [ 0 1 0  ]
  //  [ 0 0 2m ]
  // for 0 <= m <= 0.5
  // ==> We just use m instead of 2m, and let m go from 0 to 1.

  for (i=0; i<numbot1; ++i) {
    double m = (double)i / (double)(numbot1-1);
    gsl_matrix *mat = make_diag3(1, 1, m);
    calc_entropy_alpha(mat, &entropy, &alpha);
    fprintf(fp,"%f,%f\n",entropy,alpha);
    gsl_matrix_free(mat);
    asfPercentMeter((double)(i+numtop)/size);
  }

  // Generate the second part of the bottom curve by finding the eigenvalues
  // of the matrix:
  //  [ 2m-1 0 0 ]
  //  [ 0    1 0 ]
  //  [ 0    0 1 ]
  // for 0.5 <= m <= 1
  // ==> We just use m instead of 2m-1, and let m go from 0 to 1.

  for (i=0; i<numbot2; ++i) {
    double m = (double)i / (double)(numbot2-1);
    gsl_matrix *mat = make_diag3(m, 1, 1);
    calc_entropy_alpha(mat, &entropy, &alpha);
    fprintf(fp,"%f,%f\n",entropy,alpha);    
    gsl_matrix_free(mat);
    asfPercentMeter((double)(i+numtop+numbot1)/size);
  }

  asfPercentMeter(1.0);
  FCLOSE(fp);
}
