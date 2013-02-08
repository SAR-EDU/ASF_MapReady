#include "asf_iso_meta.h"
#include "asf_nan.h" // needed for MAGIC_UNSET_DOUBLE

iso_generalHeader *iso_generalHeader_init(void)
{
  iso_generalHeader *header = 
    (iso_generalHeader *) MALLOC(sizeof(iso_generalHeader));
  
  strcpy(header->itemName, MAGIC_UNSET_STRING);
  strcpy(header->mission, MAGIC_UNSET_STRING);
  strcpy(header->source, MAGIC_UNSET_STRING);
  strcpy(header->destination, MAGIC_UNSET_STRING);
  strcpy(header->generationSystem, MAGIC_UNSET_STRING);
  header->generationTime.year = MAGIC_UNSET_INT;
  header->generationTime.month = MAGIC_UNSET_INT;
  header->generationTime.day = MAGIC_UNSET_INT;
  header->generationTime.hour = MAGIC_UNSET_INT;
  header->generationTime.min = MAGIC_UNSET_INT;
  header->generationTime.second = MAGIC_UNSET_DOUBLE;
  header->referenceDocument = NULL;
  header->revision = NULL;
  header->revisionComment= NULL;
  
  return header;
}

iso_productComponents *iso_productComponents_init(void)
{
  iso_productComponents *comps = 
    (iso_productComponents *) MALLOC(sizeof(iso_productComponents));

  comps->annotation = NULL;
  comps->imageData = NULL;
  comps->auxRasterFiles = NULL;
  comps->quicklooks = NULL;
  comps->numAnnotations = 0;
  comps->numLayers = 0;
  comps->numAuxRasterFiles = 0;  
  comps->compositeQuicklook = NULL;
  strcpy(comps->browseImage.host, MAGIC_UNSET_STRING);
  strcpy(comps->browseImage.path, MAGIC_UNSET_STRING);
  strcpy(comps->browseImage.name, MAGIC_UNSET_STRING);
  comps->browseImage.size = MAGIC_UNSET_INT;
  strcpy(comps->mapPlot.host, MAGIC_UNSET_STRING);
  strcpy(comps->mapPlot.path, MAGIC_UNSET_STRING);
  strcpy(comps->mapPlot.name, MAGIC_UNSET_STRING);
  comps->mapPlot.size = MAGIC_UNSET_INT;

  return comps;
}

iso_productInfo *iso_productInfo_init(void)
{
  int ii;

  iso_productInfo *info = (iso_productInfo *) MALLOC(sizeof(iso_productInfo));

  // generationInfo
  strcpy(info->logicalProductID, MAGIC_UNSET_STRING);
  strcpy(info->receivingStation, MAGIC_UNSET_STRING);
  strcpy(info->level0ProcessingFacility, MAGIC_UNSET_STRING);
  strcpy(info->level1ProcessingFacility, MAGIC_UNSET_STRING);
  info->groundOperationsType = UNDEF_OPS;
  strcpy(info->deliveryInfo, MAGIC_UNSET_STRING);
  strcpy(info->copyrightInfo, MAGIC_UNSET_STRING);
  info->qualityInspection = UNDEF_QUALITY;
  info->qualityRemark = NULL;

  // missionInfo
  strcpy(info->mission, MAGIC_UNSET_STRING);
  info->orbitPhase = MAGIC_UNSET_INT;
  info->orbitCycle = MAGIC_UNSET_INT;
  info->absOrbit = MAGIC_UNSET_INT;
  info->relOrbit = MAGIC_UNSET_INT;
  info->numOrbitsInCycle = MAGIC_UNSET_INT;
  info->orbitDirection = UNDEF_ORBIT;

  // acquisitionInfo
  strcpy(info->sensor, MAGIC_UNSET_STRING);
  info->imageMode = UNDEF_IMAGE_MODE;
  info->lookDirection = UNDEF_LOOK;
  info->polarizationMode = UNDEF_POL_MODE;
  info->polLayer = NULL;
  strcpy(info->elevationBeamConfiguration, MAGIC_UNSET_STRING);
  strcpy(info->azimuthBeamID, MAGIC_UNSET_STRING);
  info->numberOfBeams = MAGIC_UNSET_INT;
  info->beamID = NULL;
  info->numberOfBursts = MAGIC_UNSET_INT;
  info->numberOfAzimuthBeams = MAGIC_UNSET_INT;
  strcpy(info->azimuthBeamIDFirst, MAGIC_UNSET_STRING);
  strcpy(info->azimuthBeamIDLast, MAGIC_UNSET_STRING);
  info->azimuthSteeringAngleFirst = MAGIC_UNSET_DOUBLE;
  info->azimuthSteeringAngleLast = MAGIC_UNSET_DOUBLE;

  // productVariantInfo
  strcpy(info->productType, MAGIC_UNSET_STRING);
  info->productVariant = UNDEF_PRODUCT;
  info->projection = UNDEF_PROJ;
  info->mapProjection = UNDEF_MAP;
  info->resolutionVariant = UNDEF_RES;
  info->radiometricCorrection = UNDEF_CAL;

  // imageDataInfo
  strcpy(info->pixelValueID, MAGIC_UNSET_STRING);
  info->imageDataType = UNDEF_DATA_TYPE;
  info->imageDataFormat = UNDEF_DATA_FORMAT;
  info->numberOfLayers = MAGIC_UNSET_INT;
  info->imageDataDepth = MAGIC_UNSET_INT;
  info->imageStorageOrder = UNDEF_STORE;
  strcpy(info->rowContent, MAGIC_UNSET_STRING);
  strcpy(info->columnContent, MAGIC_UNSET_STRING);
  info->numberOfRows = MAGIC_UNSET_INT;
  info->numberOfColumns = MAGIC_UNSET_INT;
  info->rowSpacing = MAGIC_UNSET_DOUBLE;
  info->columnSpacing = MAGIC_UNSET_DOUBLE;
  info->groundRangeResolution = MAGIC_UNSET_DOUBLE;
  info->azimuthResolution = MAGIC_UNSET_DOUBLE;
  info->azimuthLooks = MAGIC_UNSET_DOUBLE;
  info->rangeLooks = MAGIC_UNSET_DOUBLE;

  // sceneInfo
  strcpy(info->sceneID, MAGIC_UNSET_STRING);
  info->startTimeUTC.year = MAGIC_UNSET_INT;
  info->startTimeUTC.month = MAGIC_UNSET_INT;
  info->startTimeUTC.day = MAGIC_UNSET_INT;
  info->startTimeUTC.hour = MAGIC_UNSET_INT;
  info->startTimeUTC.min = MAGIC_UNSET_INT;
  info->startTimeUTC.second = MAGIC_UNSET_INT;
  info->startTimeGPS = MAGIC_UNSET_INT;
  info->startTimeGPSFraction = MAGIC_UNSET_DOUBLE;
  info->stopTimeUTC.year = MAGIC_UNSET_INT;
  info->stopTimeUTC.month = MAGIC_UNSET_INT;
  info->stopTimeUTC.day = MAGIC_UNSET_INT;
  info->stopTimeUTC.hour = MAGIC_UNSET_INT;
  info->stopTimeUTC.min = MAGIC_UNSET_INT;
  info->stopTimeUTC.second = MAGIC_UNSET_INT;
  info->stopTimeGPS = MAGIC_UNSET_INT;
  info->stopTimeGPSFraction = MAGIC_UNSET_DOUBLE;
  info->rangeTimeFirstPixel = MAGIC_UNSET_DOUBLE;
  info->rangeTimeLastPixel = MAGIC_UNSET_DOUBLE;
  info->sceneAzimuthExtent = MAGIC_UNSET_DOUBLE;
  info->sceneRangeExtent = MAGIC_UNSET_DOUBLE;
  info->sceneCenterCoord.refRow = NULL;
  info->sceneCenterCoord.refColumn = NULL;
  info->sceneCenterCoord.lat = MAGIC_UNSET_DOUBLE;
  info->sceneCenterCoord.lon = MAGIC_UNSET_DOUBLE;
  info->sceneCenterCoord.azimuthTimeUTC.year = MAGIC_UNSET_INT;
  info->sceneCenterCoord.azimuthTimeUTC.month = MAGIC_UNSET_INT;
  info->sceneCenterCoord.azimuthTimeUTC.day = MAGIC_UNSET_INT;
  info->sceneCenterCoord.azimuthTimeUTC.hour = MAGIC_UNSET_INT;
  info->sceneCenterCoord.azimuthTimeUTC.min = MAGIC_UNSET_INT;
  info->sceneCenterCoord.azimuthTimeUTC.second = MAGIC_UNSET_DOUBLE;
  info->sceneCenterCoord.rangeTime = MAGIC_UNSET_DOUBLE;
  info->sceneCenterCoord.incidenceAngle = MAGIC_UNSET_DOUBLE;
  info->sceneAverageHeight = MAGIC_UNSET_DOUBLE;
  for (ii=0; ii<4; ii++) {
    info->sceneCornerCoord[ii].refRow = NULL;
    info->sceneCornerCoord[ii].refColumn = NULL;
    info->sceneCornerCoord[ii].lat = MAGIC_UNSET_DOUBLE;
    info->sceneCornerCoord[ii].lon = MAGIC_UNSET_DOUBLE;
    info->sceneCornerCoord[ii].azimuthTimeUTC.year = MAGIC_UNSET_INT;
    info->sceneCornerCoord[ii].azimuthTimeUTC.month = MAGIC_UNSET_INT;
    info->sceneCornerCoord[ii].azimuthTimeUTC.day = MAGIC_UNSET_INT;
    info->sceneCornerCoord[ii].azimuthTimeUTC.hour = MAGIC_UNSET_INT;
    info->sceneCornerCoord[ii].azimuthTimeUTC.min = MAGIC_UNSET_INT;
    info->sceneCornerCoord[ii].azimuthTimeUTC.second = MAGIC_UNSET_DOUBLE;
    info->sceneCornerCoord[ii].rangeTime = MAGIC_UNSET_DOUBLE;
    info->sceneCornerCoord[ii].incidenceAngle = MAGIC_UNSET_DOUBLE;
  }
  info->headingAngle = MAGIC_UNSET_DOUBLE;

  // previewInfo
  strcpy(info->quicklooks.imageDataFormat, MAGIC_UNSET_STRING);
  info->quicklooks.imageDataDepth = MAGIC_UNSET_INT;
  info->quicklooks.numberOfRows = MAGIC_UNSET_INT;
  info->quicklooks.numberOfColumns = MAGIC_UNSET_INT;
  info->quicklooks.columnBlockLength = MAGIC_UNSET_DOUBLE;
  info->quicklooks.rowBlockLength = MAGIC_UNSET_DOUBLE;
  info->quicklooks.rowSpacing = MAGIC_UNSET_DOUBLE;
  info->quicklooks.columnSpacing = MAGIC_UNSET_DOUBLE;
  strcpy(info->compositeQLImageDataFormat, MAGIC_UNSET_STRING);
  info->compositeQLImageDataDepth = MAGIC_UNSET_INT;
  info->compositeQLPolLayerCode = NULL;
  strcpy(info->browseImageDataFormat, MAGIC_UNSET_STRING);
  info->browseImageDataDepth = MAGIC_UNSET_INT;
  strcpy(info->mapPlotFormat, MAGIC_UNSET_STRING);

  return info;
}

iso_productSpecific *iso_productSpecific_init(void)
{
  iso_productSpecific *spec = 
    (iso_productSpecific *) MALLOC(sizeof(iso_productSpecific));

  spec->commonPRF = MAGIC_UNSET_DOUBLE;
  spec->commonRSF = MAGIC_UNSET_DOUBLE;
  spec->slantRangeResolution = MAGIC_UNSET_DOUBLE;
  spec->projectedSpacingAzimuth = MAGIC_UNSET_DOUBLE;
  spec->projectedSpacingGroundNearRange = MAGIC_UNSET_DOUBLE;
  spec->projectedSpacingGroundFarRange = MAGIC_UNSET_DOUBLE;
  spec->projectedSpacingSlantRange = MAGIC_UNSET_DOUBLE;
  spec->imageCoordinateType = UNDEF_COORD;
  spec->imageDataStartWith = UNDEF_DATA_START;
  spec->quicklookDataStartWith = UNDEF_DATA_START;

  // geocodedImageInfo
  spec->geocodedImageInfoFlag = FALSE;

  // mapProjection
  strcpy(spec->geodeticDatumID, MAGIC_UNSET_STRING);
  strcpy(spec->projectionID, MAGIC_UNSET_STRING);
  strcpy(spec->zoneID, MAGIC_UNSET_STRING);
  spec->projectionCenterLatitude = MAGIC_UNSET_DOUBLE;
  spec->projectionCenterLongitude = MAGIC_UNSET_DOUBLE;
  spec->mapOriginEasting = MAGIC_UNSET_DOUBLE;
  spec->mapOriginNorthing = MAGIC_UNSET_DOUBLE;
  spec->scaleFactor = MAGIC_UNSET_DOUBLE;

  // geoParameter
  spec->pixelSpacingEasting = MAGIC_UNSET_DOUBLE;
  spec->pixelSpacingNorthing = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsGeographic.upperLeftLatitude = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsGeographic.upperLeftLongitude = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsGeographic.upperRightLatitude = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsGeographic.upperRightLongitude = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsGeographic.lowerLeftLatitude = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsGeographic.lowerLeftLongitude = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsGeographic.lowerRightLatitude = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsGeographic.lowerRightLongitude = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsCartographic.upperLeftEasting = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsCartographic.upperLeftNorthing = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsCartographic.upperRightEasting = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsCartographic.upperRightNorthing = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsCartographic.lowerRightEasting = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsCartographic.lowerRightNorthing = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsCartographic.lowerLeftEasting = MAGIC_UNSET_DOUBLE;
  spec->frameCoordsCartographic.lowerLeftNorthing = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsGeographic.upperLeftLatitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsGeographic.upperLeftLongitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsGeographic.upperRightLatitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsGeographic.upperRightLongitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsGeographic.lowerLeftLatitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsGeographic.lowerLeftLongitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsGeographic.lowerRightLatitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsGeographic.lowerRightLongitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsCartographic.upperLeftEasting = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsCartographic.upperLeftNorthing = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsCartographic.upperRightEasting = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsCartographic.upperRightNorthing = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsCartographic.lowerRightEasting = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsCartographic.lowerRightNorthing = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsCartographic.lowerLeftEasting = MAGIC_UNSET_DOUBLE;
  spec->sceneCoordsCartographic.lowerLeftNorthing = MAGIC_UNSET_DOUBLE;
  spec->sceneCenterCoordLatitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCenterCoordLongitude = MAGIC_UNSET_DOUBLE;
  spec->sceneCenterCoordEasting = MAGIC_UNSET_DOUBLE;
  spec->sceneCenterCoordNorthing = MAGIC_UNSET_DOUBLE;
  spec->imageResamplingMethod = UNDEF_RESAMPLE;

  // elevationData
  spec->elevationDataFlag = FALSE;
  strcpy(spec->elevationDataSource, MAGIC_UNSET_STRING);
  spec->elevationMinimumHeight = MAGIC_UNSET_DOUBLE;
  spec->elevationMeanHeight = MAGIC_UNSET_DOUBLE;
  spec->elevationMaximumHeight = MAGIC_UNSET_DOUBLE;

  // incidenceAngleMaskDescription
  spec->incidenceAngleMaskDescriptionFlag = FALSE;
  strcpy(spec->incidenceAnglePixelValueID, MAGIC_UNSET_STRING);
  spec->incidenceAngleImageDataFormat = UNDEF_DATA_FORMAT;
  spec->incidenceAngleImageDataDepth = MAGIC_UNSET_INT;
  spec->incidenceAngleNumberOfRows = MAGIC_UNSET_INT;
  spec->incidenceAngleNumberOfColumns = MAGIC_UNSET_INT;
  spec->incidenceAngleRowSpacing = MAGIC_UNSET_DOUBLE;
  spec->incidenceAngleColumnSpacing = MAGIC_UNSET_DOUBLE;

  return spec;
}

iso_setup *iso_setup_init(void)
{
  iso_setup *setup = (iso_setup *) MALLOC(sizeof(iso_setup));

  strcpy(setup->orderType, MAGIC_UNSET_STRING);
  strcpy(setup->processingPriority, MAGIC_UNSET_STRING);
  setup->orbitAccuracy = UNDEF_ORBIT_ACC;
  setup->sceneSpecification = UNDEF_SCENE_SPEC;
  setup->frameID = MAGIC_UNSET_INT;
  setup->sceneStartTimeUTC.year = MAGIC_UNSET_INT;
  setup->sceneStartTimeUTC.month = MAGIC_UNSET_INT;
  setup->sceneStartTimeUTC.day = MAGIC_UNSET_INT;
  setup->sceneStartTimeUTC.hour = MAGIC_UNSET_INT;
  setup->sceneStartTimeUTC.min = MAGIC_UNSET_INT;
  setup->sceneStartTimeUTC.second = MAGIC_UNSET_DOUBLE;
  setup->sceneStopTimeUTC.year = MAGIC_UNSET_INT;
  setup->sceneStopTimeUTC.month = MAGIC_UNSET_INT;
  setup->sceneStopTimeUTC.day = MAGIC_UNSET_INT;
  setup->sceneStopTimeUTC.hour = MAGIC_UNSET_INT;
  setup->sceneStopTimeUTC.min = MAGIC_UNSET_INT;
  setup->sceneStopTimeUTC.second = MAGIC_UNSET_DOUBLE;
  setup->sceneCenterLatitude = MAGIC_UNSET_DOUBLE;
  setup->sceneCenterLongitude = MAGIC_UNSET_DOUBLE;
  setup->imagingMode = UNDEF_IMAGE_MODE;
  setup->lookDirection = UNDEF_LOOK;
  setup->polarizationMode = UNDEF_POL_MODE;
  setup->polLayer = UNDEF_POL_LAYER;
  strcpy(setup->elevationBeamConfiguration, MAGIC_UNSET_STRING);
  setup->productVariant = UNDEF_PRODUCT;
  setup->resolutionVariant = UNDEF_RES;
  setup->projection = UNDEF_PROJ;
  strcpy(setup->logicalDataTakeID, MAGIC_UNSET_STRING);
  strcpy(setup->level0ProductID, MAGIC_UNSET_STRING);
  setup->L0SARGenerationTimeUTC.year = MAGIC_UNSET_INT;
  setup->L0SARGenerationTimeUTC.month = MAGIC_UNSET_INT;
  setup->L0SARGenerationTimeUTC.day = MAGIC_UNSET_INT;
  setup->L0SARGenerationTimeUTC.hour = MAGIC_UNSET_INT;
  setup->L0SARGenerationTimeUTC.min = MAGIC_UNSET_INT;
  setup->L0SARGenerationTimeUTC.second = MAGIC_UNSET_DOUBLE;
  setup->numProcessingSteps = 0;
  setup->processingStep = NULL;

  return setup;
}

iso_processing *iso_processing_init(void)
{
  iso_processing *proc = (iso_processing *) MALLOC(sizeof(iso_processing));

  strcpy(proc->dopplerBasebandEstimationMethod, MAGIC_UNSET_STRING);
  proc->dopplerCentroidCoordinateType = UNDEF_COORD;
  proc->doppler = NULL;
  proc->processingParameter = NULL;  
  proc->chirpReplicaUsedFlag = FALSE;
  proc->geometricDopplerUsedFlag = FALSE;
  proc->azimuthPatternCorrectedFlag = FALSE;
  proc->elevationPatternCorrectedFlag = FALSE;
  proc->detectedFlag = FALSE;
  proc->multiLookedFlag = FALSE;
  proc->polarimetricProcessedFlag = FALSE;
  proc->terrainCorrectedFlag = FALSE;
  proc->layoverShadowMaskGeneratedFlag = FALSE;
  proc->geocodedFlag = FALSE;
  proc->nominalProcessingPerformedFlag = FALSE;

  return proc;
}

iso_instrument *iso_instrument_init(void) 
{
  iso_instrument *inst = (iso_instrument *) MALLOC(sizeof(iso_instrument));

  inst->instrumentInfoCoordinateType = UNDEF_COORD;
  inst->centerFrequency = MAGIC_UNSET_DOUBLE;
  inst->numSettings = 0;
  inst->settings = NULL;

  return inst;
}

iso_calibration *iso_calibration_init(void);

iso_platform *iso_platform_init(void)
{
  iso_platform *platform = (iso_platform *) MALLOC(sizeof(iso_platform));

  platform->sensor = UNDEF_ORBIT_SENSOR;
  platform->accuracy = UNDEF_ORBIT_ACC;
  platform->numStateVectors = 0;
  platform->firstStateTimeUTC.year = MAGIC_UNSET_INT;
  platform->firstStateTimeUTC.month = MAGIC_UNSET_INT;
  platform->firstStateTimeUTC.day = MAGIC_UNSET_INT;
  platform->firstStateTimeUTC.hour = MAGIC_UNSET_INT;
  platform->firstStateTimeUTC.min = MAGIC_UNSET_INT;
  platform->firstStateTimeUTC.second = MAGIC_UNSET_DOUBLE;
  platform->lastStateTimeUTC.year = MAGIC_UNSET_INT;
  platform->lastStateTimeUTC.month = MAGIC_UNSET_INT;
  platform->lastStateTimeUTC.day = MAGIC_UNSET_INT;
  platform->lastStateTimeUTC.hour = MAGIC_UNSET_INT;
  platform->lastStateTimeUTC.min = MAGIC_UNSET_INT;
  platform->lastStateTimeUTC.second = MAGIC_UNSET_DOUBLE;
  strcpy(platform->stateVectorRefFrame, MAGIC_UNSET_STRING);
  platform->stateVectorTimeSpacing = MAGIC_UNSET_DOUBLE;
  platform->stateVec = NULL;

  return platform;
}

iso_productQuality *iso_productQuality_init(void)
{
  iso_productQuality *quality = 
    (iso_productQuality *) MALLOC(sizeof(iso_productQuality));

  quality->rawDataQuality = NULL;
  quality-> dopplerAmbiguityNotZeroFlag = FALSE;
  quality->dopplerOutsideLimitsFlag = FALSE;
  quality->geolocationQualityLowFlag = FALSE;
  quality->imageDataQuality = NULL;
  quality->gapDefinition = MAGIC_UNSET_INT;
  quality->gapPercentageLimit = MAGIC_UNSET_DOUBLE;
  quality->missingLinePercentageLimit = MAGIC_UNSET_DOUBLE;
  quality->bitErrorLimit = MAGIC_UNSET_DOUBLE;
  quality->timeReconstructionPercentageLimit = MAGIC_UNSET_DOUBLE;
  quality->dopplerCentroidLimit = MAGIC_UNSET_DOUBLE;
  quality->geolocationQualityLimit = MAGIC_UNSET_DOUBLE;
  quality->instrumentStateRemark = NULL;

  return quality;
}

iso_meta *iso_meta_init(void)
{
  iso_meta *iso = (iso_meta *) MALLOC(sizeof(iso_meta));
  
  iso->generalHeader     = iso_generalHeader_init();
  iso->productComponents = iso_productComponents_init();
  iso->productInfo       = iso_productInfo_init();
  iso->productSpecific   = iso_productSpecific_init();
  iso->setup             = iso_setup_init();
  iso->processing        = iso_processing_init();
  iso->instrument        = iso_instrument_init();
  iso->calibration       = NULL; // currently only uncalibrated Seasat data
  iso->platform          = iso_platform_init();
  iso->productQuality    = iso_productQuality_init();;
  
  iso->meta_version = ISO_META_VERSION;
  
  return iso;
}

void iso_meta_free(iso_meta *iso)
{
  int ii;

  if (iso != NULL) {
    if (iso->generalHeader) {
      if (iso->generalHeader->referenceDocument)
	FREE(iso->generalHeader->referenceDocument);
      if (iso->generalHeader->revision)
	FREE(iso->generalHeader->revision);
      if (iso->generalHeader->revisionComment)
	FREE(iso->generalHeader->revisionComment);
      FREE(iso->generalHeader);
      iso->generalHeader = NULL;
    }
    if (iso->productComponents) {
      if (iso->productComponents->annotation)
	FREE(iso->productComponents->annotation);
      if (iso->productComponents->imageData)
	FREE(iso->productComponents->imageData);
      if (iso->productComponents->auxRasterFiles)
	FREE(iso->productComponents->auxRasterFiles);
      if (iso->productComponents->quicklooks)
	FREE(iso->productComponents->quicklooks);
      if (iso->productComponents->compositeQuicklook)
	FREE(iso->productComponents->compositeQuicklook);
      FREE(iso->productComponents);
      iso->productComponents = NULL;
    }
    if (iso->productInfo) {
      if (iso->productInfo->qualityRemark)
	FREE(iso->productInfo->qualityRemark);
      if (iso->productInfo->polLayer)
	FREE(iso->productInfo->polLayer);
      if (iso->productInfo->imageMode == SCANSAR_IMAGE) {
	for (ii=0; ii<iso->productInfo->numberOfBeams; ii++)
	  FREE(iso->productInfo->beamID[ii]);
	FREE(iso->productInfo->beamID);
      }
      if (iso->productInfo->sceneCenterCoord.refRow)
	FREE(iso->productInfo->sceneCenterCoord.refRow);
      if (iso->productInfo->sceneCenterCoord.refColumn)
	FREE(iso->productInfo->sceneCenterCoord.refColumn);
      for (ii=0; ii<4; ii++) {
	if (iso->productInfo->sceneCornerCoord[ii].refRow)
	  FREE(iso->productInfo->sceneCornerCoord[ii].refRow);
	if (iso->productInfo->sceneCornerCoord[ii].refColumn)
	  FREE(iso->productInfo->sceneCornerCoord[ii].refColumn);
      }
      if (iso->productInfo->compositeQLPolLayerCode)
	FREE(iso->productInfo->compositeQLPolLayerCode);
      FREE(iso->productInfo);
      iso->productInfo = NULL;
    }
    if (iso->productSpecific) {
      FREE(iso->productSpecific);
      iso->productSpecific = NULL;
    }
    if (iso->setup) {
      if (iso->setup->numProcessingSteps > 0) {
	for (ii=0; ii<iso->setup->numProcessingSteps; ii++)
	  if (iso->setup->processingStep[ii].algorithm)
	    FREE(iso->setup->processingStep[ii].algorithm);
	FREE(iso->setup->processingStep);
      }
      FREE(iso->setup);
      iso->setup = NULL;
    }
    if (iso->instrument) {
      if (iso->instrument->settings) {
	for (ii=0; ii<iso->instrument->numSettings; ii++)
	  FREE(iso->instrument->settings[ii].settingRecord);
      }
      FREE(iso->instrument);
      iso->instrument = NULL;
    }
    if (iso->platform) {
      if (iso->platform->stateVec)
	FREE(iso->platform->stateVec);
      FREE(iso->platform);
      iso->platform = NULL;
    }
    if (iso->productQuality) {
      if (iso->productQuality->rawDataQuality)
	FREE(iso->productQuality->rawDataQuality);
      if (iso->productQuality->imageDataQuality)
	FREE(iso->productQuality->imageDataQuality);
      if (iso->productQuality->instrumentStateRemark)
	FREE(iso->productQuality->instrumentStateRemark);
      FREE(iso->productQuality);
      iso->productQuality = NULL;
    }
    FREE(iso);
    iso = NULL;
  }
}
