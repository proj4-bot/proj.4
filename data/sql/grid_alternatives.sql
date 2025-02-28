INSERT INTO grid_alternatives(original_grid_name,
                              proj_grid_name,
                              old_proj_grid_name,
                              proj_grid_format,
                              proj_method,
                              inverse_direction,
                              package_name,
                              url, direct_download, open_license, directory)
VALUES

-- ar_ign - Instituto Geográfico Nacional (IGN)
('GEOIDE-Ar16.gri','ar_ign_GEOIDE-Ar16.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ar_ign_GEOIDE-Ar16.tif',1,1,NULL),

-- at_bev - Austria Bundesamt für Eich- und Vermessungswessen
('AT_GIS_GRID.gsb','at_bev_AT_GIS_GRID.tif','AT_GIS_GRID.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/at_bev_AT_GIS_GRID.tif',1,1,NULL),
('AT_GIS_GRID_2021_09_28.gsb','at_bev_AT_GIS_GRID_2021_09_28.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/at_bev_AT_GIS_GRID_2021_09_28.tif',1,1,NULL),
('GV_HoehenGrid_V1.csv','at_bev_GV_Hoehengrid_V1.tif',NULL,'GTiff','vgridshift',0,NULL,'https://cdn.proj.org/at_bev_GV_Hoehengrid_V1.tif',1,1,NULL),
('GEOID_GRS80_Oesterreich.csv','at_bev_GEOID_GRS80_Oesterreich.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/at_bev_GEOID_GRS80_Oesterreich.tif',1,1,NULL),
('GEOID_BESSEL_Oesterreich.csv','at_bev_GEOID_BESSEL_Oesterreich.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/at_bev_GEOID_BESSEL_Oesterreich.tif',1,1,NULL),
('GV_Hoehengrid_plus_Geoid_V3.csv','at_bev_GV_Hoehengrid_plus_Geoid_V2.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/at_bev_GV_Hoehengrid_plus_Geoid_V2.tif',1,1,NULL),

-- au_ga - Geoscience Australia
-- source file contains undulation in first band, and deflection in 2nd and 3d band
('AUSGeoid09_V1.01.gsb','au_ga_AUSGeoid09_V1.01.tif','AUSGeoid09_V1.01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/au_ga_AUSGeoid09_V1.01.tif',1,1,NULL),
('AUSGeoid09_GDA94_V1.01_DOV_windows.gsb','au_ga_AUSGeoid09_V1.01.tif','AUSGeoid09_V1.01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/au_ga_AUSGeoid09_V1.01.tif',1,1,NULL),
-- source file contains undulation in first band, and deflection in 2nd and 3d band
('AUSGeoid2020_20180201.gsb','au_ga_AUSGeoid2020_20180201.tif','AUSGeoid2020_20180201.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/au_ga_AUSGeoid2020_20180201.tif',1,1,NULL),
('AGQG_20191107.gsb','au_ga_AGQG_20191107.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/au_ga_AGQG_20191107.tif',1,1,NULL),
('AGQG_20201120.gsb','au_ga_AGQG_20201120.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/au_ga_AGQG_20201120.tif',1,1,NULL),

-- au_icsm - Australian Intergovernmental Committee on Surveying and Mapping
('A66 National (13.09.01).gsb','au_icsm_A66_National_13_09_01.tif','A66_National_13_09_01.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/au_icsm_A66_National_13_09_01.tif',1,1,NULL),
('National 84 (02.07.01).gsb','au_icsm_National_84_02_07_01.tif','National_84_02_07_01.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/au_icsm_National_84_02_07_01.tif',1,1,NULL),
('GDA94_GDA2020_conformal.gsb','au_icsm_GDA94_GDA2020_conformal.tif','GDA94_GDA2020_conformal.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/au_icsm_GDA94_GDA2020_conformal.tif',1,1,NULL),
('GDA94_GDA2020_conformal_and_distortion.gsb','au_icsm_GDA94_GDA2020_conformal_and_distortion.tif','GDA94_GDA2020_conformal_and_distortion.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/au_icsm_GDA94_GDA2020_conformal_and_distortion.tif',1,1,NULL),
('GDA94_GDA2020_conformal_christmas_island.gsb','au_icsm_GDA94_GDA2020_conformal_christmas_island.tif','GDA94_GDA2020_conformal_christmas_island.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/au_icsm_GDA94_GDA2020_conformal_christmas_island.tif',1,1,NULL),
('GDA94_GDA2020_conformal_cocos_island.gsb','au_icsm_GDA94_GDA2020_conformal_cocos_island.tif','GDA94_GDA2020_conformal_cocos_island.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/au_icsm_GDA94_GDA2020_conformal_cocos_island.tif',1,1,NULL),

-- be_ign - IGN Belgium
('bd72lb72_etrs89lb08.gsb','be_ign_bd72lb72_etrs89lb08.tif','bd72lb72_etrs89lb08.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/be_ign_bd72lb72_etrs89lb08.tif',1,1,NULL),
('hBG18.dat', 'be_ign_hBG18.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/be_ign_hBG18.tif',1,1,NULL),

-- br_ibge - Instituto Brasileiro de Geografia e Estatistica (IBGE)
('CA61_003.gsb','br_ibge_CA61_003.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/br_ibge_CA61_003.tif',1,1,NULL),
('CA7072_003.gsb','br_ibge_CA7072_003.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/br_ibge_CA7072_003.tif',1,1,NULL),
('SAD69_003.gsb','br_ibge_SAD69_003.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/br_ibge_SAD69_003.tif',1,1,NULL),
('SAD96_003.gsb','br_ibge_SAD96_003.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/br_ibge_SAD96_003.tif',1,1,NULL),

-- ca_nrc - Natural Resources Canada
('CGG2013ai08.byn','ca_nrc_CGG2013ai08.tif','CGG2013ai08.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ca_nrc_CGG2013ai08.tif',1,1,NULL),
('CGG2013an83.byn','ca_nrc_CGG2013an83.tif','CGG2013an83.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ca_nrc_CGG2013an83.tif',1,1,NULL),
('CGG2013i08.byn','ca_nrc_CGG2013i08.tif','CGG2013i08.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ca_nrc_CGG2013i08.tif',1,1,NULL),
('CGG2013n83.byn','ca_nrc_CGG2013n83.tif','CGG2013n83.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ca_nrc_CGG2013n83.tif',1,1,NULL),
('HT2_0.byn','ca_nrc_HT2_1997.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ca_nrc_HT2_1997.tif',1,1,NULL),
('HT2_1997.byn','ca_nrc_HT2_1997.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ca_nrc_HT2_1997.tif',1,1,NULL),
('HT2_2002v70.byn','ca_nrc_HT2_2002v70.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ca_nrc_HT2_2002v70.tif',1,1,NULL),
('HT2_2010v70.byn','ca_nrc_HT2_2010v70.tif','HT2_2010v70.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ca_nrc_HT2_2010v70.tif',1,1,NULL),
('HT2_1997_CGG2013a.byn','ca_nrc_HT2_1997_CGG2013a.tif',NULL,'GTiff','vgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_HT2_1997_CGG2013a.tif',1,1,NULL),
('HT2_2002v70_CGG2013a.byn','ca_nrc_HT2_2002v70_CGG2013a.tif',NULL,'GTiff','vgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_HT2_2002v70_CGG2013a.tif',1,1,NULL),
('HT2_2010v70_CGG2013a.byn','ca_nrc_HT2_2010v70_CGG2013a.tif',NULL,'GTiff','vgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_HT2_2010v70_CGG2013a.tif',1,1,NULL),
-- the PROJ grid is the reverse way of the EPSG one
('NTv1_0.gsb','ca_nrc_ntv1_can.tif','ntv1_can.dat','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_ntv1_can.tif',1,1,NULL),
-- just a case change
('NTv2_0.gsb','ca_nrc_ntv2_0.tif','ntv2_0.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_ntv2_0.tif',1,1,NULL),
-- Provincial grids
('AB_CSRS.DAC','ca_nrc_ABCSRSV4.tif','ABCSRSV4.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_ABCSRSV4.tif',1,1,NULL),
('BC_27_05.GSB','ca_nrc_BC_27_05.tif','BC_27_05.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_BC_27_05.tif',1,1,NULL),
('BC_93_05.GSB','ca_nrc_BC_93_05.tif','BC_93_05.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_BC_93_05.tif',1,1,NULL),
('CGQ77-98.gsb','ca_nrc_CQ77SCRS.tif','CQ77SCRS.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_CQ77SCRS.tif',1,1,NULL),
('CRD27_00.GSB','ca_nrc_CRD27_00.tif','CRD27_00.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_CRD27_00.tif',1,1,NULL),
('CRD93_00.GSB','ca_nrc_CRD93_00.tif','CRD93_00.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_CRD93_00.tif',1,1,NULL),
('GS7783.GSB','ca_nrc_GS7783.tif','GS7783.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_GS7783.tif',1,1,NULL),
-- just a case change
('May76v20.gsb','ca_nrc_MAY76V20.tif','MAY76V20.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_MAY76V20.tif',1,1,NULL),
('NA27SCRS.GSB','ca_nrc_NA27SCRS.tif','NA27SCRS.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_NA27SCRS.tif',1,1,NULL),
('QUE27-98.gsb','ca_nrc_NA27SCRS.tif','NA27SCRS.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_NA27SCRS.tif',1,1,NULL),
-- two grid names in EPSG point to the same file distributed by NRCan
('NA83SCRS.GSB','ca_nrc_NA83SCRS.tif','NA83SCRS.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_NA83SCRS.tif',1,1,NULL),
('NAD83-98.gsb','ca_nrc_NA83SCRS.tif','NA83SCRS.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_NA83SCRS.tif',1,1,NULL),
('NB2783v2.gsb','ca_nrc_NB2783v2.tif','NB2783v2.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_NB2783v2.tif',1,1,NULL),
('NB7783v2.gsb','ca_nrc_NB7783v2.tif','NB7783v2.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_NB7783v2.tif',1,1,NULL),
('NS778302.gsb','ca_nrc_NS778302.tif','NS778302.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_NS778302.tif',1,1,NULL),
('NVI93_05.GSB','ca_nrc_NVI93_05.tif','NVI93_05.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_NVI93_05.tif',1,1,NULL),
('ON27CSv1.GSB','ca_nrc_ON27CSv1.tif','ON27CSv1.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_ON27CSv1.tif',1,1,NULL),
('ON76CSv1.GSB','ca_nrc_ON76CSv1.tif','ON76CSv1.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_ON76CSv1.tif',1,1,NULL),
('ON83CSv1.GSB','ca_nrc_ON83CSv1.tif','ON83CSv1.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_ON83CSv1.tif',1,1,NULL),
('PE7783V2.gsb','ca_nrc_PE7783V2.tif','PE7783V2.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_PE7783V2.tif',1,1,NULL),
('SK27-98.gsb','ca_nrc_SK27-98.tif','SK27-98.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_SK27-98.tif',1,1,NULL),
('SK83-98.gsb','ca_nrc_SK83-98.tif','SK83-98.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_SK83-98.tif',1,1,NULL),
('TO27CSv1.GSB','ca_nrc_TO27CSv1.tif','TO27CSv1.GSB','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_nrc_TO27CSv1.tif',1,1,NULL),
('NAD83v6VG.gvb','ca_nrc_NAD83v6VG.tif',NULL,'GTiff','velocity_grid',0,NULL,'https://cdn.proj.org/ca_nrc_NAD83v6VG.tif',1,1,NULL),
('NAD83v70VG.gvb','ca_nrc_NAD83v70VG.tif',NULL,'GTiff','velocity_grid',0,NULL,'https://cdn.proj.org/ca_nrc_NAD83v70VG.tif',1,1,NULL),

-- ca_que - Ministère de l'Énergie et des Ressources naturelles du Québec
-- two grid names in EPSG point to the same file distributed by NRCan
('NA27NA83.GSB','ca_que_mern_na27na83.tif','na27na83.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_que_mern_na27na83.tif',1,1,NULL),
('CQ77NA83.GSB','ca_que_mern_cq77na83.tif','cq77na83.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ca_que_mern_cq77na83.tif',1,1,NULL),

-- ch_swisstopo - Swisstopo Federal Office of Topography
('CHENyx06a.gsb','ch_swisstopo_CHENyx06a.tif','CHENyx06a.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ch_swisstopo_CHENyx06a.tif',1,1,NULL),
('CHENyx06_ETRS.gsb','ch_swisstopo_CHENyx06_ETRS.tif','CHENyx06_ETRS.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/ch_swisstopo_CHENyx06_ETRS.tif',1,1,NULL),
('chgeo2004_ETRS.agr','ch_swisstopo_chgeo2004_ETRS89_LHN95.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ch_swisstopo_chgeo2004_ETRS89_LHN95.tif',1,1,NULL),
('chgeo2004_htrans_ETRS.agr','ch_swisstopo_chgeo2004_ETRS89_LN02.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/ch_swisstopo_chgeo2004_ETRS89_LN02.tif',1,1,NULL),

-- cz_cuzk - ČÚZK
('CR2005_GTX.gtx','cz_cuzk_CR-2005.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/cz_cuzk_CR-2005.tif',1,1,NULL),

-- de_adv - Arbeitsgemeinschaft der Vermessungsverwaltungender der Länder der Bundesrepublik Deutschland (AdV)
('BETA2007.gsb','de_adv_BETA2007.tif','BETA2007.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/de_adv_BETA2007.tif',1,1,NULL),

-- de_bkg - Bundesamt für Kartographie und Geodäsie (BKG)
('GCG2016.txt','de_bkg_gcg2016.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/de_bkg_gcg2016.tif',1,1,NULL),

-- de_geosn - Staatsbetrieb Geobasisinformation und Vermessung Sachsen GeoSN
('NTv2_SN.gsb','de_geosn_NTv2_SN.tif','NTv2_SN.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/de_geosn_NTv2_SN.tif',1,1,NULL),

-- de_hvbg - Hessische Verwaltung für Bodenmanagement und Geoinformation
('HeTa2010.gsb','de_hvbg_hessen_HeTA2010.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/de_hvbg_hessen_HeTA2010.tif',1,1,NULL),

-- de_lgl_bw - LGL Baden-Württemberg
('BWTA2017.gsb','de_lgl_bw_BWTA2017.tif','BWTA2017.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/de_lgl_bw_BWTA2017.tif',1,1,NULL),

-- de_lgvl_saarland - LVGL Saarland
('SeTa2016.gsb','de_lgvl_saarland_SeTa2016.tif','SeTa2016.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/de_lgvl_saarland_SeTa2016.tif',1,1,NULL),

-- de_tlbg_thueringen - TLBG Thüringen
('de_tlbg_thuringen_NTv2gridTH.gsb','de_tlbg_thueringen_NTv2gridTH.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/de_tlbg_thueringen_NTv2gridTH.tif',1,1,NULL),

-- dk_sdfe - Danish Agency for Data Supply and Efficiency
-- Denmark mainland
('dnn.gtx','dk_sdfe_dnn.tif','dnn.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfe_dnn.tif',1,1,NULL),
--  Faroe islands height models
('fvr09.gtx','dk_sdfe_fvr09.tif','fvr09.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfe_fvr09.tif',1,1,NULL),
-- Greenland height models
('gr2000g.gri','dk_sdfe_gvr2000.tif','gvr2000.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfe_gvr2000.tif',1,1,NULL),
('ggeoid16.gri','dk_sdfe_gvr2016.tif','gvr2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfe_gvr2016.tif',1,1,NULL),

-- dk_sdfi - Danish Agency for Data Supply and Infrastructure
-- Denmark mainland
('gs_2022.gsb','dk_sdfi_gs_2022.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/dk_sdfi_gs_2022.tif',1,1,NULL),
('gsb_2022.gsb','dk_sdfi_gsb_2022.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/dk_sdfi_gsb_2022.tif',1,1,NULL),
('kk_2022.gsb','dk_sdfi_kk_2022.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/dk_sdfi_kk_2022.tif',1,1,NULL),
('os_2022.gsb','dk_sdfi_os_2022.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/dk_sdfi_os_2022.tif',1,1,NULL),
('s34j_2022.gsb','dk_sdfi_s34j_2022.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/dk_sdfi_s34j_2022.tif',1,1,NULL),
('s34s_2022.gsb','dk_sdfi_s34s_2022.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/dk_sdfi_s34s_2022.tif',1,1,NULL),
('s45b_2022.gsb','dk_sdfi_s45b_2022.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/dk_sdfi_s45b_2022.tif',1,1,NULL),
('dvr90_2002.tif','dk_sdfi_dvr90_2002.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_dvr90_2002.tif',1,1,NULL),
('dvr90_2013.tif','dk_sdfi_dvr90_2013.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_dvr90_2013.tif',1,1,NULL),
('dvr90_2023.tif','dk_sdfi_dvr90_2023.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_dvr90_2023.tif',1,1,NULL),
('dklat_2022.tif','dk_sdfi_dklat_2022.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_dklat_2022.tif',1,1,NULL),
('dklat_2023.tif','dk_sdfi_dklat_2023.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_dklat_2023.tif',1,1,NULL),
('dkmsl_2022.tif','dk_sdfi_dkmsl_2022.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_dkmsl_2022.tif',1,1,NULL),
('dkmsl_2023.tif','dk_sdfi_dkmsl_2023.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_dkmsl_2023.tif',1,1,NULL),
-- Greenland
('gllmsl_2022.tif','dk_sdfi_gllmsl_2022.tif',NULL,'GTiff','vgridshift',0,NULL,'https://cdn.proj.org/dk_sdfi_gllmsl_2022.tif',1,1,NULL),
('gllat_2023.tif','dk_sdfi_gllat_2023.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_gllat_2023.tif',1,1,NULL),
('glmsl_2023.tif','dk_sdfi_glmsl_2023.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/dk_sdfi_glmsl_2023.tif',1,1,NULL),

-- es_cat_icgc - Institut Cartogràfic i Geològic de Catalunya (ICGC)
('100800401.gsb','es_cat_icgc_100800401.tif','100800401.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/es_cat_icgc_100800401.tif',1,1,NULL),

-- es_ign - Instituto Geográfico Nacional (IGN)
('SPED2ETV2.gsb','es_ign_SPED2ETV2.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/es_ign_SPED2ETV2.tif',1,1,NULL),
('EGM08_REDNAP.txt','es_ign_egm08-rednap.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/es_ign_egm08-rednap.tif',1,1,NULL),
('EGM08_REDNAP_Canarias.txt','es_ign_egm08-rednap-canarias.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/es_ign_egm08-rednap-canarias.tif',1,1,NULL),

-- eur_nkg - Nordic Geodetic Commission
('eur_nkg_nkgrf03vel_realigned.tif','eur_nkg_nkgrf03vel_realigned.tif',NULL,'GTiff','velocity_grid',0,NULL,'https://cdn.proj.org/eur_nkg_nkgrf03vel_realigned.tif',1,1,NULL),
('eur_nkg_nkgrf17vel.tif','eur_nkg_nkgrf17vel.tif',NULL,'GTiff','velocity_grid',0,NULL,'https://cdn.proj.org/eur_nkg_nkgrf17vel.tif',1,1,NULL),

-- fi_nls - National Land Survey of Finland (MML)
('fi_nls_fin2000.tif','fi_nls_fin2000.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fi_nls_fin2000.tif',1,1,NULL),
('fi_nls_fin2005n00.tif','fi_nls_fin2005n00.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fi_nls_fin2005n00.tif',1,1,NULL),
('fi_nls_fin2023n2000.tif','fi_nls_fin2023n2000.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fi_nls_fin2023n2000.tif',1,1,NULL),
('fi_nls_n43_n60.json','fi_nls_n43_n60.json',NULL,'JSON','tinshift',0,NULL,'https://cdn.proj.org/fi_nls_n43_n60.json',1,1,NULL),
('fi_nls_n60_n2000.json','fi_nls_n60_n2000.json',NULL,'JSON','tinshift',0,NULL,'https://cdn.proj.org/fi_nls_n60_n2000.json',1,1,NULL),
('fi_nls_ykj_etrs35fin.json','fi_nls_ykj_etrs35fin.json',NULL,'JSON','tinshift',0,NULL,'https://cdn.proj.org/fi_nls_ykj_etrs35fin.json',1,1,NULL),

-- fr_ign - IGN France
('rgf93_ntf.gsb','fr_ign_ntf_r93.tif','ntf_r93.gsb','GTiff','hgridshift',1,NULL,'https://cdn.proj.org/fr_ign_ntf_r93.tif',1,1,NULL),
('gr3df97a.txt','fr_ign_gr3df97a.tif',NULL,'GTiff','geocentricoffset',0,NULL,'https://cdn.proj.org/fr_ign_gr3df97a.tif',1,1,NULL),
('RGM04versRGM23.txt','fr_ign_RGM04versRGM23.tif',NULL,'GTiff','geocentricoffset',0,NULL,'https://cdn.proj.org/fr_ign_RGM04versRGM23.tif',1,1,NULL),
-- Vertical grids
('RAC09.mnt','fr_ign_RAC09.tif','RAC09.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAC09.tif',1,1,NULL),
('RAC23.mnt','fr_ign_RAC23.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAC23.tif',1,1,NULL),
('RAF09.mnt','fr_ign_RAF09.tif','RAF09.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAF09.tif',1,1,NULL),
('RAF18.mnt','fr_ign_RAF18.tif','RAF18.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAF18.tif',1,1,NULL),
('RAF18b.mnt','fr_ign_RAF18b.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAF18b.tif',1,1,NULL),
('RAF20.tac','fr_ign_RAF20.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAF20.tif',1,1,NULL),
-- The following is a bit an abusive mapping. The ggf97a.txt is the one pointed for the "RGF93 v1 to NGF-IGN69 height" transformation
-- but we have never ingested that grid. So we point to RAF18.mnt instead...
-- If we didn't do this, we wouldn't get a grid at all, since RAF09 and RAF18 are now only available for RGF93 v2
('ggf97a.txt','fr_ign_RAF18.tif','RAF18.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAF18.tif',1,1,NULL),
('gg10_gtbt.txt','fr_ign_RAGTBT2016.tif','RAGTBT2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAGTBT2016.tif',1,1,NULL),
('RAGTBT2016.mnt','fr_ign_RAGTBT2016.tif','RAGTBT2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAGTBT2016.tif',1,1,NULL),
('gg10_ld.txt','fr_ign_RALD2016.tif','RALD2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RALD2016.tif',1,1,NULL),
('RALD2016.mnt','fr_ign_RALD2016.tif','RALD2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RALD2016.tif',1,1,NULL),
('ggg00_ld.txt','fr_ign_RALDW842016.tif','RALDW842016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RALDW842016.tif',1,1,NULL),
('RALDW842016.mnt','fr_ign_RALDW842016.tif','RALDW842016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RALDW842016.tif',1,1,NULL),
('gg10_ls.txt','fr_ign_RALS2016.tif','RALS2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RALS2016.tif',1,1,NULL),
('RALS2016.mnt','fr_ign_RALS2016.tif','RALS2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RALS2016.tif',1,1,NULL),
('gg10_mart.txt','fr_ign_RAMART2016.tif','RAMART2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAMART2016.tif',1,1,NULL),
('RAMART2016.mnt','fr_ign_RAMART2016.tif','RAMART2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAMART2016.tif',1,1,NULL),
('gg10_mg.txt','fr_ign_RAMG2016.tif','RAMG2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAMG2016.tif',1,1,NULL),
('RAMG2016.mnt','fr_ign_RAMG2016.tif','RAMG2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAMG2016.tif',1,1,NULL),
('ggr99.txt','fr_ign_RAR07_bl.tif','RAR07_bl.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RAR07_bl.tif',1,1,NULL),
('RASPM2018.mnt','fr_ign_RASPM2018.tif','RASPM2018.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_RASPM2018.tif',1,1,NULL),
('gg10_sb.txt','fr_ign_gg10_sbv2.tif','gg10_sbv2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_gg10_sbv2.tif',1,1,NULL),
('gg10_sbv2.mnt','fr_ign_gg10_sbv2.tif','gg10_sbv2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_gg10_sbv2.tif',1,1,NULL),
('gg10_sm.txt','fr_ign_gg10_smv2.tif','gg10_smv2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_gg10_smv2.tif',1,1,NULL),
('gg10_smv2.mnt','fr_ign_gg10_smv2.tif','gg10_smv2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_gg10_smv2.tif',1,1,NULL),
('ggg00_ls.txt','fr_ign_ggg00_lsv2.tif','ggg00_lsv2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggg00_lsv2.tif',1,1,NULL),
('ggg00_mg.txt','fr_ign_ggg00_mgv2.tif','ggg00_mgv2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggg00_mgv2.tif',1,1,NULL),
('ggg00_sb.txt','fr_ign_ggg00_sbv2.tif','ggg00_sbv2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggg00_sbv2.tif',1,1,NULL),
('ggg00_sm.txt','fr_ign_ggg00_smv2.tif','ggg00_smv2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggg00_smv2.tif',1,1,NULL),
('ggg00.txt','fr_ign_ggg00v2.tif','ggg00v2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggg00v2.tif',1,1,NULL),
('ggguy00.txt','fr_ign_ggguy15.tif','ggguy15.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggguy15.tif',1,1,NULL),
('ggm00.txt','fr_ign_ggm00v2.tif','ggm00v2.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggm00v2.tif',1,1,NULL),
('GGSPM06v1.mnt','fr_ign_ggspm06v1.tif','ggspm06v1.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggspm06v1.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_CGVD2013RGSPM06.tif','fr_ign_CGVD2013RGSPM06.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_CGVD2013RGSPM06.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf02-Maiao.tif','fr_ign_ggpf02-Maiao.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf02-Maiao.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf02-Tupai.tif','fr_ign_ggpf02-Tupai.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf02-Tupai.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf05-HivaOa.tif','fr_ign_ggpf05-HivaOa.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf05-HivaOa.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf05-Nuku.tif','fr_ign_ggpf05-Nuku.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf05-Nuku.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf08-Gambier.tif','fr_ign_ggpf08-Gambier.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf08-Gambier.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf08-Hao.tif','fr_ign_ggpf08-Hao.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf08-Hao.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf08-Mataiva.tif','fr_ign_ggpf08-Mataiva.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf08-Mataiva.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf08-Raivavae.tif','fr_ign_ggpf08-Raivavae.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf08-Raivavae.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf08-Reao.tif','fr_ign_ggpf08-Reao.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf08-Reao.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf08-Rurutu.tif','fr_ign_ggpf08-Rurutu.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf08-Rurutu.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf08-Tikehau.tif','fr_ign_ggpf08-Tikehau.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf08-Tikehau.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-fr_ign_ggpf08-Tubuai.tif','fr_ign_ggpf08-Tubuai.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/fr_ign_ggpf08-Tubuai.tif',1,1,NULL),

-- hu_bme - Budapest University of Technology and Economics
('hu_bme_geoid2014.tif','hu_bme_geoid2014.tif', 'geoid_eht2014.gtx', 'GTiff', 'geoid_like', 0, NULL, 'https://cdn.proj.org/hu_bme_geoid2014.tif', 1, 1, NULL),
('hu_bme_hd72corr.gsb', 'hu_bme_hd72corr.tif', 'etrs2eov_notowgs.gsb', 'GTiff', 'hgridshift', 0, NULL, 'https://cdn.proj.org/hu_bme_hd72corr.tif', 1, 1, NULL),

-- is_lmi - National Land Survey of Iceland
('Icegeoid_ISN2004.gri','is_lmi_Icegeoid_ISN2004.tif','Icegeoid_ISN2004.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/is_lmi_Icegeoid_ISN2004.tif',1,1,NULL),
('Icegeoid_ISN93.gri','is_lmi_Icegeoid_ISN93.tif','Icegeoid_ISN93.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/is_lmi_Icegeoid_ISN93.tif',1,1,NULL),
('Icegeoid_ISN2016.gri','is_lmi_Icegeoid_ISN2016.tif','Icegeoid_ISN2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/is_lmi_Icegeoid_ISN2016.tif',1,1,NULL),
('ISN93_ISN2016.gsb','is_lmi_ISN93_ISN2016.tif','ISN93_ISN2016.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/is_lmi_ISN93_ISN2016.tif',1,1,NULL),
('ISN2004_ISN2016.gsb','is_lmi_ISN2004_ISN2016.tif','ISN2004_ISN2016.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/is_lmi_ISN2004_ISN2016.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-is_lmi_ISN_vel_beta.tif','is_lmi_ISN_vel_beta.tif',NULL,'GTiff','velocity_grid',0,NULL,'https://cdn.proj.org/is_lmi_ISN_vel_beta.tif',1,1,NULL),

-- jp_gsi - Geospatial Information Authority of Japan
('jp_gsi_gsigeo2011.tif','jp_gsi_gsigeo2011.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/jp_gsi_gsigeo2011.tif',1,1,NULL),

-- lv_lgia - Latvian Geospatial Information Agency
('lv_lgia_lv14.tif','lv_lgia_lv14.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/lv_lgia_lv14.tif',1,1,NULL),

-- mx_inegi - Instituto Nacional de Estadística, Geografía e Informática (INEGI) Mexico
('GGM10.txt','mx_inegi_ggm10.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/mx_inegi_ggm10.tif',1,1,NULL),

-- nc_dittt - Gouvernement de Nouvelle Calédonie - DITTT
('Ranc08_Circe.mnt','nc_dittt_Ranc08_Circe.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/nc_dittt_Ranc08_Circe.tif',1,1,NULL),
('RANC15.tac','nc_dittt_RANC15.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/nc_dittt_RANC15.tif',1,1,NULL),
('gr3dnc01b.mnt','nc_dittt_gr3dnc01b.tif',NULL,'GTiff','geocentricoffset',0,NULL,'https://cdn.proj.org/nc_dittt_gr3dnc01b.tif',1,1,NULL),
('gr3dnc02b.mnt','nc_dittt_gr3dnc02b.tif',NULL,'GTiff','geocentricoffset',0,NULL,'https://cdn.proj.org/nc_dittt_gr3dnc02b.tif',1,1,NULL),
('gr3dnc03a.mnt','nc_dittt_gr3dnc03a.tif',NULL,'GTiff','geocentricoffset',0,NULL,'https://cdn.proj.org/nc_dittt_gr3dnc03a.tif',1,1,NULL),
('gr3dncl08.tac','nc_dittt_gr3dncI08.tif',NULL,'GTiff','geocentricoffset',0,NULL,'https://cdn.proj.org/nc_dittt_gr3dncI08.tif',1,1,NULL),

-- Netherlands / RDNAP (non-free grids). See https://salsa.debian.org/debian-gis-team/proj-rdnap/raw/master/debian/copyright
-- Netherlands / RDNAP 2018
('nlgeo2018.gtx','nl_nsgi_nlgeo2018.tif','nlgeo2018.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/nl_nsgi_nlgeo2018.tif',1,1,NULL),
('rdtrans2018.gsb','nl_nsgi_rdtrans2018.tif','rdtrans2018.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/nl_nsgi_rdtrans2018.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-naptrans2018.gtx','nl_nsgi_naptrans2018.tif','naptrans2018.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/nl_nsgi_naptrans2018.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-rdcorr2018.gsb','nl_nsgi_rdcorr2018.tif','rdcorr2018.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/nl_nsgi_rdcorr2018.tif',1,1,NULL),
('naptrans2008.gtx','','naptrans2008.gtx','GTX','geoid_like',0,NULL,'https://salsa.debian.org/debian-gis-team/proj-rdnap/raw/upstream/2008/naptrans2008.gtx',1,0,NULL),
('rdtrans2008.gsb','','rdtrans2008.gsb','NTv2','hgridshift',0,NULL,'https://salsa.debian.org/debian-gis-team/proj-rdnap/raw/upstream/2008/rdtrans2008.gsb',1,0,NULL),

-- no_kv - Kartverket
-- Norwegian grids
('HREF2018B_NN2000_EUREF89.bin','no_kv_HREF2018B_NN2000_EUREF89.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/no_kv_HREF2018B_NN2000_EUREF89.tif',1,1,NULL),
('href2008a.bin','no_kv_href2008a.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/no_kv_href2008a.tif',1,1,NULL),
('no_kv_NKGETRF14_EPSG7922_2000.tif','no_kv_NKGETRF14_EPSG7922_2000.tif',NULL,'GTiff','geocentricoffset',0,NULL,'https://cdn.proj.org/no_kv_NKGETRF14_EPSG7922_2000.tif',1,1,NULL),
('ChartDatum_above_Ellipsoid_EUREF89_v2021a.bin','no_kv_CD_above_Ell_ETRS89_v2021a.tif',NULL,'GTiff','vgridshift',0,NULL,'https://cdn.proj.org/no_kv_CD_above_Ell_ETRS89_v2021a.tif',1,1,NULL),
('ChartDatum_above_Ellipsoid_EUREF89_v2023b.bin','no_kv_CD_above_Ell_ETRS89_v2023b.tif',NULL,'GTiff','vgridshift',0,NULL,'https://cdn.proj.org/no_kv_CD_above_Ell_ETRS89_v2023b.tif',1,1,NULL),
('no_kv_ETRS89NO_NGO48_TIN.json','no_kv_ETRS89NO_NGO48_TIN.json',NULL,'JSON','tinshift',0,NULL,'https://cdn.proj.org/no_kv_ETRS89NO_NGO48_TIN.json',1,1,NULL),
('arcgp-2006-sk.bin','no_kv_arcgp-2006-sk.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/no_kv_arcgp-2006-sk.tif',1,1,NULL),

-- nz_linz - New Zealand
('nzgd2kgrid0005.gsb','nz_linz_nzgd2kgrid0005.tif','nzgd2kgrid0005.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/nz_linz_nzgd2kgrid0005.tif',1,1,NULL),
('nzgeoid2016.gtx','nz_linz_nzgeoid2016.tif','nzgeoid2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/nz_linz_nzgeoid2016.tif',1,1,NULL),
('nzgd2000_deformation_20000101_full.zip','nz_linz_nzgd2000-20000101.json',NULL,'GTiff','defmodel',0,NULL,'https://cdn.proj.org/nz_linz_nzgd2000-20000101.json',1,1,NULL),
('nzgd2000_deformation_20130801_full.zip','nz_linz_nzgd2000-20130801.json',NULL,'GTiff','defmodel',0,NULL,'https://cdn.proj.org/nz_linz_nzgd2000-20130801.json',1,1,NULL),
('nzgd2000_deformation_20140201_full.zip','nz_linz_nzgd2000-20140201.json',NULL,'GTiff','defmodel',0,NULL,'https://cdn.proj.org/nz_linz_nzgd2000-20140201.json',1,1,NULL),
('nzgd2000_deformation_20150101_full.zip','nz_linz_nzgd2000-20150101.json',NULL,'GTiff','defmodel',0,NULL,'https://cdn.proj.org/nz_linz_nzgd2000-20150101.json',1,1,NULL),
('nzgd2000_deformation_20160701_full.zip','nz_linz_nzgd2000-20160701.json',NULL,'GTiff','defmodel',0,NULL,'https://cdn.proj.org/nz_linz_nzgd2000-20160701.json',1,1,NULL),
('nzgd2000_deformation_20171201_full.zip','nz_linz_nzgd2000-20171201.json',NULL,'GTiff','defmodel',0,NULL,'https://cdn.proj.org/nz_linz_nzgd2000-20171201.json',1,1,NULL),
('nzgd2000_deformation_20180701_full.zip','nz_linz_nzgd2000-20180701.json',NULL,'GTiff','defmodel',0,NULL,'https://cdn.proj.org/nz_linz_nzgd2000-20180701.json',1,1,NULL),
-- Superseded
('New_Zealand_Quasigeoid_2016.csv','nz_linz_nzgeoid2016.tif','nzgeoid2016.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/nz_linz_nzgeoid2016.tif',1,1,NULL),
('nzgeoid2009.gtx','nz_linz_nzgeoid2009.tif','nzgeoid2009.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/nz_linz_nzgeoid2009.tif',1,1,NULL),
-- Superseded
('nzgeoid09.sid','nz_linz_nzgeoid2009.tif','nzgeoid2009.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/nz_linz_nzgeoid2009.tif',1,1,NULL),
-- New Zealand grid shift models.
('auckht1946-nzvd2016.gtx','nz_linz_auckht1946-nzvd2016.tif','auckht1946-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_auckht1946-nzvd2016.tif',1,1,NULL),
('blufht1955-nzvd2016.gtx','nz_linz_blufht1955-nzvd2016.tif','blufht1955-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_blufht1955-nzvd2016.tif',1,1,NULL),
('dublht1960-nzvd2016.gtx','nz_linz_dublht1960-nzvd2016.tif','dublht1960-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_dublht1960-nzvd2016.tif',1,1,NULL),
('duneht1958-nzvd2016.gtx','nz_linz_duneht1958-nzvd2016.tif','duneht1958-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_duneht1958-nzvd2016.tif',1,1,NULL),
('gisbht1926-nzvd2016.gtx','nz_linz_gisbht1926-nzvd2016.tif','gisbht1926-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_gisbht1926-nzvd2016.tif',1,1,NULL),
('lyttht1937-nzvd2016.gtx','nz_linz_lyttht1937-nzvd2016.tif','lyttht1937-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_lyttht1937-nzvd2016.tif',1,1,NULL),
('motuht1953-nzvd2016.gtx','nz_linz_motuht1953-nzvd2016.tif','motuht1953-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_motuht1953-nzvd2016.tif',1,1,NULL),
('napiht1962-nzvd2016.gtx','nz_linz_napiht1962-nzvd2016.tif','napiht1962-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_napiht1962-nzvd2016.tif',1,1,NULL),
('nelsht1955-nzvd2016.gtx','nz_linz_nelsht1955-nzvd2016.tif','nelsht1955-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_nelsht1955-nzvd2016.tif',1,1,NULL),
('ontpht1964-nzvd2016.gtx','nz_linz_ontpht1964-nzvd2016.tif','ontpht1964-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_ontpht1964-nzvd2016.tif',1,1,NULL),
('stisht1977-nzvd2016.gtx','nz_linz_stisht1977-nzvd2016.tif','stisht1977-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_stisht1977-nzvd2016.tif',1,1,NULL),
('taraht1970-nzvd2016.gtx','nz_linz_taraht1970-nzvd2016.tif','taraht1970-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_taraht1970-nzvd2016.tif',1,1,NULL),
('wellht1953-nzvd2016.gtx','nz_linz_wellht1953-nzvd2016.tif','wellht1953-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_wellht1953-nzvd2016.tif',1,1,NULL),
-- Superseded entries
('auckland-1946-to-nzvd2016-conversion.csv','nz_linz_auckht1946-nzvd2016.tif','auckht1946-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_auckht1946-nzvd2016.tif',1,1,NULL),
('bluff-1955-to-nzvd2016-conversion.csv','nz_linz_blufht1955-nzvd2016.tif','blufht1955-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_blufht1955-nzvd2016.tif',1,1,NULL),
('dunedin-bluff-1960-to-nzvd2016-conversion.csv','nz_linz_dublht1960-nzvd2016.tif','dublht1960-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_dublht1960-nzvd2016.tif',1,1,NULL),
('dunedin-1958-to-nzvd2016-conversion.csv','nz_linz_duneht1958-nzvd2016.tif','duneht1958-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_duneht1958-nzvd2016.tif',1,1,NULL),
('gisborne-1926-to-nzvd2016-conversion.csv','nz_linz_gisbht1926-nzvd2016.tif','gisbht1926-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_gisbht1926-nzvd2016.tif',1,1,NULL),
('lyttelton-1937-to-nzvd2016-conversion.csv','nz_linz_lyttht1937-nzvd2016.tif','lyttht1937-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_lyttht1937-nzvd2016.tif',1,1,NULL),
('moturiki-1953-to-nzvd2016-conversion.csv','nz_linz_motuht1953-nzvd2016.tif','motuht1953-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_motuht1953-nzvd2016.tif',1,1,NULL),
('napier-1962-to-nzvd2016-conversion.csv','nz_linz_napiht1962-nzvd2016.tif','napiht1962-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_napiht1962-nzvd2016.tif',1,1,NULL),
('nelson-1955-to-nzvd2016-conversion.csv','nz_linz_nelsht1955-nzvd2016.tif','nelsht1955-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_nelsht1955-nzvd2016.tif',1,1,NULL),
('onetreepoint-1964-to-nzvd2016-conversion.csv','nz_linz_ontpht1964-nzvd2016.tif','ontpht1964-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_ontpht1964-nzvd2016.tif',1,1,NULL),
('stewartisland-1977-to-nzvd2016-conversion.csv','nz_linz_stisht1977-nzvd2016.tif','stisht1977-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_stisht1977-nzvd2016.tif',1,1,NULL),
('taranaki-1970-to-nzvd2016-conversion.csv','nz_linz_taraht1970-nzvd2016.tif','taraht1970-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_taraht1970-nzvd2016.tif',1,1,NULL),
('wellington-1953-to-nzvd2016-conversion.csv','nz_linz_wellht1953-nzvd2016.tif','wellht1953-nzvd2016.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/nz_linz_wellht1953-nzvd2016.tif',1,1,NULL),

-- pl_gugik - Główny Urząd Geodezji i Kartografii
('gugik-geoid2011-PL-EVRF2007-NH.txt','pl_gugik_geoid2011-PL-EVRF2007-NH.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/pl_gugik_geoid2011-PL-EVRF2007-NH.tif',1,1,NULL),
('gugik-geoid2011-PL-KRON86-NH.txt','pl_gugik_geoid2011-PL-KRON86-NH.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/pl_gugik_geoid2011-PL-KRON86-NH.tif',1,1,NULL),
('Model_quasi-geoidy-PL-geoid2021-PL-EVRF2007-NH.txt','pl_gugik_geoid2021-PL-EVRF2007-NH.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/pl_gugik_geoid2021-PL-EVRF2007-NH.tif',1,1,NULL),

-- pt_dgt - DG Territorio
('DLx_ETRS89_geo.gsb','pt_dgt_DLx_ETRS89_geo.tif','DLx_ETRS89_geo.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/pt_dgt_DLx_ETRS89_geo.tif',1,1,NULL),
('D73_ETRS89_geo.gsb','pt_dgt_D73_ETRS89_geo.tif','D73_ETRS89_geo.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/pt_dgt_D73_ETRS89_geo.tif',1,1,NULL),
('GeodPT08.dat','pt_dgt_GeodPT08.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/pt_dgt_GeodPT08.tif',1,1,NULL),

-- se_lantmateriet - Sweden
('SWEN17_RH2000.gtx','se_lantmateriet_SWEN17_RH2000.tif','SWEN17_RH2000.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/se_lantmateriet_SWEN17_RH2000.tif',1,1,NULL),

-- si_gurs - Slovenia
('https://isgeoid.polimi.it/Geoid/Europe/Slovenia/public/Slovenia_2016_SLO_VRP2016_Koper_hybrQ_20221122.isg','si_gurs_SLO-VRP2016-Koper.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/si_gurs_SLO-VRP2016-Koper.tif',1,1,NULL),

-- sk_gku - Geodetický a kartografický ústav Bratislava (GKU)
('Slovakia_JTSK03_to_JTSK.LAS','sk_gku_JTSK03_to_JTSK.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/sk_gku_JTSK03_to_JTSK.tif',1,1,NULL),
('Slovakia_ETRS89h_to_Baltic1957.gtx','sk_gku_Slovakia_ETRS89h_to_Baltic1957.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/sk_gku_Slovakia_ETRS89h_to_Baltic1957.tif',1,1,NULL),
('Slovakia_ETRS89h_to_EVRF2007.gtx','sk_gku_Slovakia_ETRS89h_to_EVRF2007.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/sk_gku_Slovakia_ETRS89h_to_EVRF2007.tif',1,1,NULL),

-- uk_os - Ordnance Survey
-- Northern Ireland: OSGM15 height, Belfast height -> ETRS89 ellipsoidal heights
('OSGM15_Belfast.gri','uk_os_OSGM15_Belfast.tif','OSGM15_Belfast.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/uk_os_OSGM15_Belfast.tif',1,1,NULL),
-- United Kingdom: OSGM15 height, ODN height -> ETRS89 ellipsoidal heights
('OSTN15_OSGM15_GB.txt','uk_os_OSGM15_GB.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/uk_os_OSGM15_GB.tif',1,1,NULL),
-- Ireland: OSGM15 height, Malin head datum -> ETRS89 ellipsoidal heights
('OSGM15_Malin.gri','uk_os_OSGM15_Malin.tif','OSGM15_Malin.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/uk_os_OSGM15_Malin.tif',1,1,NULL),
('OSTN15_NTv2_OSGBtoETRS.gsb','uk_os_OSTN15_NTv2_OSGBtoETRS.tif','OSTN15_NTv2_OSGBtoETRS.gsb','GTiff','hgridshift',0,NULL,'https://cdn.proj.org/uk_os_OSTN15_NTv2_OSGBtoETRS.tif',1,1,NULL),

-- us_nga - US National Geospatial Intelligence Agency (NGA)
('WW15MGH.GRD','us_nga_egm96_15.tif','egm96_15.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_nga_egm96_15.tif',1,1,NULL),
('Und_min2.5x2.5_egm2008_isw=82_WGS84_TideFree.gz','us_nga_egm08_25.tif','egm08_25.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_nga_egm08_25.tif',1,1,NULL),

-- us_noaa - United States
-- Continental USA VERTCON: NGVD (19)29 height to NAVD (19)88 height
('vertconw.94','us_noaa_vertconw.tif','vertconw.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/us_noaa_vertconw.tif',1,1,NULL),
('vertconc.94','us_noaa_vertconc.tif','vertconc.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/us_noaa_vertconc.tif',1,1,NULL),
('vertcone.94','us_noaa_vertcone.tif','vertcone.gtx','GTiff','vgridshift',0,NULL,'https://cdn.proj.org/us_noaa_vertcone.tif',1,1,NULL),
-- US GEOID99 height models. Not mapped: Alaska: g1999a01.gtx to g1999a04.gtx. Hawaii: g1999h01.gtx, Puerto Rico: g1999p01.gtx
('NOT-YET-IN-GRID-TRANSFORMATION-g1999a01.bin','us_noaa_g1999a01.tif','g1999a01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999a01.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g1999a02.bin','us_noaa_g1999a02.tif','g1999a02.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999a02.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g1999a03.bin','us_noaa_g1999a03.tif','g1999a03.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999a03.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g1999a04.bin','us_noaa_g1999a04.tif','g1999a04.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999a04.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g1999p01.bin','us_noaa_g1999p01.tif','g1999p01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999p01.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g1999h01.bin','us_noaa_g1999h01.tif','g1999h01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999h01.tif',1,1,NULL),
('g1999u01.bin','us_noaa_g1999u01.tif','g1999u01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999u01.tif',1,1,NULL),
('g1999u02.bin','us_noaa_g1999u02.tif','g1999u02.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999u02.tif',1,1,NULL),
('g1999u03.bin','us_noaa_g1999u03.tif','g1999u03.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999u03.tif',1,1,NULL),
('g1999u04.bin','us_noaa_g1999u04.tif','g1999u04.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999u04.tif',1,1,NULL),
('g1999u05.bin','us_noaa_g1999u05.tif','g1999u05.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999u05.tif',1,1,NULL),
('g1999u06.bin','us_noaa_g1999u06.tif','g1999u06.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999u06.tif',1,1,NULL),
('g1999u07.bin','us_noaa_g1999u07.tif','g1999u07.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999u07.tif',1,1,NULL),
('g1999u08.bin','us_noaa_g1999u08.tif','g1999u08.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g1999u08.tif',1,1,NULL),
-- US GEOID03 height models. Not mapped: Alaska: g2003a01.gtx to g2003a04.gtx. Hawaii: g2003h01.gtx. Puerto Rico: g2003p01.gtx
('NOT-YET-IN-GRID-TRANSFORMATION-g2003a01.bin','us_noaa_g2003a01.tif','g2003a01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2003a01.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g2003a02.bin','us_noaa_g2003a02.tif','g2003a02.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2003a02.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g2003a03.bin','us_noaa_g2003a03.tif','g2003a03.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2003a03.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g2003a04.bin','us_noaa_g2003a04.tif','g2003a04.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2003a04.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g2003p01.bin','us_noaa_g2003p01.tif','g2003p01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2003p01.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g2003h01.bin','us_noaa_g2003h01.tif','g2003h01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2003h01.tif',1,1,NULL),('geoid03_conus.bin','us_noaa_geoid03_conus.tif','geoid03_conus.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_geoid03_conus.tif',1,1,NULL),
-- US GEOID06 height models
('geoid06_ak.bin','us_noaa_geoid06_ak.tif','geoid06_ak.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_geoid06_ak.tif',1,1,NULL),
-- US GEOID09 height models.Not mapped: Hawaii: g2009h01.gtx
('geoid09_ak.bin','us_noaa_geoid09_ak.tif','geoid09_ak.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_geoid09_ak.tif',1,1,NULL),
('geoid09_conus.bin','us_noaa_geoid09_conus.tif','geoid09_conus.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_geoid09_conus.tif',1,1,NULL),
('g2009g01.bin','us_noaa_g2009g01.tif','g2009g01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2009g01.tif',1,1,NULL),
('g2009s01.bin','us_noaa_g2009s01.tif','g2009s01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2009s01.tif',1,1,NULL),
('g2009p01.bin','us_noaa_g2009p01.tif','g2009p01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2009p01.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-g2009h01.bin','us_noaa_g2009h01.tif','g2009h01.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2009h01.tif',1,1,NULL),
-- US GEOID12B height models
-- CONUS
('g2012bu0.bin','us_noaa_g2012bu0.tif','g2012bu0.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2012bu0.tif',1,1,NULL),
-- Alaska
('g2012ba0.bin','us_noaa_g2012ba0.tif','g2012ba0.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2012ba0.tif',1,1,NULL),
-- Puerto Rico
('g2012bp0.bin','us_noaa_g2012bp0.tif','g2012bp0.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2012bp0.tif',1,1,NULL),
-- Guam
('g2012bg0.bin','us_noaa_g2012bg0.tif','g2012bg0.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2012bg0.tif',1,1,NULL),
-- American Samoa
('g2012bs0.bin','us_noaa_g2012bs0.tif','g2012bs0.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2012bs0.tif',1,1,NULL),
-- Hawaii
('NOT-YET-IN-GRID-TRANSFORMATION-g2012bh0.bin','us_noaa_g2012bh0.tif','g2012bh0.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2012bh0.tif',1,1,NULL),

-- US GEOID18 height models
('g2018u0.bin','us_noaa_g2018u0.tif','g2018u0.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2018u0.tif',1,1,NULL),
('g2018p0.bin','us_noaa_g2018p0.tif','g2018p0.gtx','GTiff','geoid_like',0,NULL,'https://cdn.proj.org/us_noaa_g2018p0.tif',1,1,NULL),

('NOT-YET-IN-GRID-TRANSFORMATION-us_noaa_flhpgn.tif','us_noaa_flhpgn.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/us_noaa_flhpgn.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-us_noaa_mdhpgn.tif','us_noaa_mdhpgn.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/us_noaa_mdhpgn.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-us_noaa_tnhpgn.tif','us_noaa_tnhpgn.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/us_noaa_tnhpgn.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-us_noaa_wihpgn.tif','us_noaa_wihpgn.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/us_noaa_wihpgn.tif',1,1,NULL),
('NOT-YET-IN-GRID-TRANSFORMATION-us_noaa_wohpgn.tif','us_noaa_wohpgn.tif',NULL,'GTiff','hgridshift',0,NULL,'https://cdn.proj.org/us_noaa_wohpgn.tif',1,1,NULL),

-- za_cdngi - Chief Directorate: National Geospatial Information (CD:NGI) South Africa
('SAGEOID2010.dat','za_cdngi_sageoid2010.tif',NULL,'GTiff','geoid_like',0,NULL,'https://cdn.proj.org/za_cdngi_sageoid2010.tif',1,1,NULL)
;
