-- This file is hand generated.

-- Denmark

INSERT INTO "grid_transformation" VALUES(
    'PROJ','EPSG_4937_TO_EPSG_5733','ETRS89 to DNN height',
    NULL,
    'EPSG','9665','Geographic3D to GravityRelatedHeight (gtx)',
    'EPSG','4937', -- source CRS (ETRS89)
    'EPSG','5733', -- target CRS (DNN height)
    NULL,
    'EPSG','8666','Geoid (height correction) model file','dnn.gtx',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);

INSERT INTO "usage" VALUES(
    'PROJ',
    'EPSG_4937_TO_EPSG_5733_USAGE',
    'grid_transformation',
    'PROJ',
    'EPSG_4937_TO_EPSG_5733',
    'EPSG','3237', -- area of use: Denmark onshore
    'EPSG','1024'  -- unknown
);

-- Faroe Islands

INSERT INTO "grid_transformation" VALUES(
    'PROJ','EPSG_4937_TO_EPSG_5317','ETRS89 to FVR09 height',
    NULL,
    'EPSG','9665','Geographic3D to GravityRelatedHeight (gtx)',
    'EPSG','4937', -- source CRS (ETRS89)
    'EPSG','5317', -- target CRS (FVR09 height)
    NULL,
    'EPSG','8666','Geoid (height correction) model file','fvr09.gtx',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);

INSERT INTO "usage" VALUES(
    'PROJ',
    'EPSG_4937_TO_EPSG_5317_USAGE',
    'grid_transformation',
    'PROJ',
    'EPSG_4937_TO_EPSG_5317',
    'EPSG','3248', -- area of use: Faroe Islands - onshore
    'EPSG','1024'  -- unknown
);

-- Sweden

INSERT INTO "grid_transformation" VALUES(
    'PROJ','EPSG_4977_TO_EPSG_5613','SWEREF99 to RH2000 height',
    NULL,
    'EPSG','9665','Geographic3D to GravityRelatedHeight (gtx)',
    'EPSG','4977', -- source CRS (SWEREF99)
    'EPSG','5613', -- target CRS (RH2000 height)
    NULL,
    'EPSG','8666','Geoid (height correction) model file','SWEN17_RH2000.gtx',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);

INSERT INTO "usage" VALUES(
    'PROJ',
    'EPSG_4977_TO_EPSG_5613_USAGE',
    'grid_transformation',
    'PROJ',
    'EPSG_4977_TO_EPSG_5613',
    'EPSG','3313', -- area of use: Sweden onshore
    'EPSG','1024'  -- unknown
);

-- Japan

INSERT INTO "grid_transformation" VALUES(
    'PROJ','EPSG_6667_TO_EPSG_6695','JDG2011 to JGD2011 height',
    NULL,
    'EPSG','9665','Geographic3D to GravityRelatedHeight (gtx)',
    'EPSG','6667', -- source CRS (JDG2011)
    'EPSG','6695', -- target CRS (JDG2011 (vertical) height)
    NULL,
    'EPSG','8666','Geoid (height correction) model file','jp_gsi_gsigeo2011.tif',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);

INSERT INTO "usage" VALUES(
    'PROJ',
    'EPSG_6667_TO_EPSG_6695_USAGE',
    'grid_transformation',
    'PROJ',
    'EPSG_6667_TO_EPSG_6695',
    'EPSG','3263', -- area of use: Japan - onshore mainland
    'EPSG','1024'  -- unknown
);

-- Switzerland

INSERT INTO "grid_transformation" VALUES(
    'PROJ','EPSG_4937_TO_EPSG_5729','ETRS89 to LHN95 height',
    NULL,
    'EPSG','9665','Geographic3D to GravityRelatedHeight (gtx)',
    'EPSG','4937', -- source CRS (ETRS89)
    'EPSG','5729', -- target CRS (LHN95 height)
    NULL,
    'EPSG','8666','Geoid (height correction) model file','chgeo2004_ETRS.agr',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);

INSERT INTO "usage" VALUES(
    'PROJ',
    'EPSG_4937_TO_EPSG_5729_USAGE',
    'grid_transformation',
    'PROJ',
    'EPSG_4937_TO_EPSG_5729',
    'EPSG','1286', -- area of use: Europe - Liechtenstein and Switzerland
    'EPSG','1024'  -- unknown
);

INSERT INTO "grid_transformation" VALUES(
    'PROJ','EPSG_4937_TO_EPSG_5728','ETRS89 to LN02 height',
    NULL,
    'EPSG','9665','Geographic3D to GravityRelatedHeight (gtx)',
    'EPSG','4937', -- source CRS (ETRS89)
    'EPSG','5728', -- target CRS (LN02 height)
    NULL,
    'EPSG','8666','Geoid (height correction) model file','chgeo2004_htrans_ETRS.agr',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);

INSERT INTO "usage" VALUES(
    'PROJ',
    'EPSG_4937_TO_EPSG_5728_USAGE',
    'grid_transformation',
    'PROJ',
    'EPSG_4937_TO_EPSG_5728',
    'EPSG','1286', -- area of use: Europe - Liechtenstein and Switzerland
    'EPSG','1024'  -- unknown
);

-- Mexico

INSERT INTO "grid_transformation" VALUES(
    'PROJ','EPSG_6364_TO_EPSG_5703','Mexico ITRF2008 to NAVD88 height',
    NULL,
    'EPSG','9665','Geographic3D to GravityRelatedHeight (gtx)',
    'EPSG','6364', -- source CRS (Mexico ITRF2008)
    'EPSG','5703', -- target CRS (NAVD88 height)
    NULL,
    'EPSG','8666','Geoid (height correction) model file','GGM10.txt',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);

INSERT INTO "usage" VALUES(
    'PROJ',
    'EPSG_6364_TO_EPSG_5703_USAGE',
    'grid_transformation',
    'PROJ',
    'EPSG_6364_TO_EPSG_5703',
    'EPSG','3278', -- area of use: Mexico - onshore
    'EPSG','1024'  -- unknown
);

-- Latvia

INSERT INTO "grid_transformation" VALUES(
    'PROJ','EPSG_4949_TO_EPSG_7700','LKS-92 to Latvia 2000 height',
    NULL,
    'EPSG','9665','Geographic3D to GravityRelatedHeight (gtx)',
    'EPSG','4949', -- source CRS (LKS-92)
    'EPSG','7700', -- target CRS (Latvia 2000 height)
    NULL,
    'EPSG','8666','Geoid (height correction) model file','lv_lgia_lv14.tif',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);

INSERT INTO "usage" VALUES(
    'PROJ',
    'EPSG_4949_TO_EPSG_7700_USAGE',
    'grid_transformation',
    'PROJ',
    'EPSG_4949_TO_EPSG_7700',
    'EPSG','3268', -- area of use: Latvia - onshore
    'EPSG','1024'  -- unknown
);