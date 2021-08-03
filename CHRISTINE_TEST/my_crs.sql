INSERT INTO geodetic_crs VALUES('HOBU','GEODETIC_CRS_MY_CRS','unknown','','geographic 2D','EPSG','6424','EPSG','6326',NULL,0);
INSERT INTO usage VALUES('HOBU','USAGE_GEODETIC_CRS_MY_CRS','geodetic_crs','HOBU','GEODETIC_CRS_MY_CRS','PROJ','EXTENT_UNKNOWN','PROJ','SCOPE_UNKNOWN');
INSERT INTO conversion VALUES('HOBU','CONVERSION_MY_CRS','unknown','','EPSG','9805','Mercator (variant B)','EPSG','8823','Latitude of 1st standard parallel',5,'EPSG','9122','EPSG','8802','Longitude of natural origin',0,'EPSG','9122','EPSG','8806','False easting',0,'EPSG','9001','EPSG','8807','False northing',0,'EPSG','9001',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);
INSERT INTO usage VALUES('HOBU','USAGE_CONVERSION_MY_CRS','conversion','HOBU','CONVERSION_MY_CRS','PROJ','EXTENT_UNKNOWN','PROJ','SCOPE_UNKNOWN');
INSERT INTO projected_crs VALUES('HOBU','MY_CRS','my_crs','','EPSG','4400','HOBU','GEODETIC_CRS_MY_CRS','HOBU','CONVERSION_MY_CRS',NULL,0);
INSERT INTO usage VALUES('HOBU','USAGE_PROJECTED_CRS_MY_CRS','projected_crs','HOBU','MY_CRS','PROJ','EXTENT_UNKNOWN','PROJ','SCOPE_UNKNOWN');
