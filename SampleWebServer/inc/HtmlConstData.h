# if ! defined( HtmlConstDataH )
#	define HtmlConstDataH	/* only include me once */

/*
Copyright (c) 2012-2017 MFBS, Fedor Malakhov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef BaseHtmlConstDataH
#include "BaseHtmlConstData.h"
#endif

extern char GenPageShowSection[];
extern char GenPageShowItem[];
extern char GenPageShowLinkItem[];
extern char GenPageChgSectionInfo[];
extern char GenPageAboutServer[];
extern char GenPageForUser[];
extern char GenPageMyContacts[];
extern char GenPageShowUserBasket[];
extern char GenPageAddItemBasket[];
extern char GenPageDeleteItemBasket[];
extern char GenPageChgNumItemBasket[];
extern char GenPageUserRegRequest[];
extern char GenPageAboutShop[];
extern char GenPageFillOrder[];
extern char GenPageUserContactInfoSet[];
extern char GenPageMyOrders[];
extern char GenPageShowUserOrder[];
extern char GenPageNonUserOrderSubmit[];
extern char GenPageOrderDBManage[];
extern char GenPageSectionDBManage[];
extern char GenPageRegClientsList[];
extern char GenPageServerConfig[];
extern char GenPagePasswdSentMail[];
extern char GenPageDeliveryInfo[];
extern char GenPagePaymentInfo[];
extern char GenPageServStats[];
extern char GenPageLogMgr[];
extern char GenPageActSessList[];
extern char GenPageBrandDBManage[];
extern char GenPageBrandInfo[];
extern char GenPageBrandIdInfo[];
extern char GenPageAdmPswdChg[];
extern char GenPageColorDBManage[];
extern char GenPageImageShow[];
extern char GenPageImageIdShow[];
extern char GenPageShowBrandFilter[];
extern char GenPageChgBrandFilterInfo[];
extern char GenPageFindCmpltBrandFilter[];
extern char GenPageFeedbackShow[];
extern char GenPageAddFeedback[];
extern char GenPageFeedbackSet[];
extern char GenPageFeedbackDBManage[];
extern char GenPageItemShowCount[];
extern char GenPageShowItemsBrand[];
extern char GenPageShowDiscItems[];
extern char GenPageChgDiscItemsInfo[];
extern char GenPageDelItemOrder[];
extern char GenPageSearchEngene[];
extern char GenPageChgSearchItemsInfo[];
extern char GenPageDelUser[];
extern char GenPageChgUserType[];
extern char GenPageBackUsrInfo[];
extern char GenPageBackReqDBManage[];
extern char GenPageCghBackReqDBManage[];
extern char GenPageSalesAnalyse[];
extern char GenPageCghPgSalesAnalyse[];
extern char GenPageLogToCsvExport[];
extern char GenPageShowExportLogList[];
extern char GenPageDynLinkStatusReq[];
extern char GenPageDynNumActUsersReq[];
extern char GenPageStatsDBManage[];
extern char GenPageCghStatsDbManage[];
extern char GenPageOamServStatsReq[];
extern char GenPageCghOamStatsServReqSent[];
extern char GenPageStatsToCsvExport[];
extern char GenPageStatsAnalyseReq[];
extern char GenPageOamServRTStatsReq[];
extern char GenPageRTStatsShowReq[];
extern char GenPageDynRTStatsDataReq[];
extern char GenPageRTStatsCollectStop[];
extern char GenPageHostDBManage[];
extern char GenPageCghHostDbManage[];
extern char GenPageClientsStatusReq[];
extern char GenPageCghClientsStatus[];
extern char GenPageCghHostsStatus[];
extern char GenPageDynAlarmListReq[];
extern char GenPageHostDeleteReq[];
extern char GenPageActAlarmShowReq[];
extern char GenPageCghActiveAlmList[];
extern char GenPageActionDBManage[];
extern char GenPageActionExecReq[];
extern char GenPageStartActionExecReq[] ;
extern char GenPageRoomDBManage[];
extern char GenPageCghRoomDbManage[];
extern char GenPageRoomsStatusReq[];
extern char GenPageCghRoomsStatus[];
extern char GenPageRoomDeleteReq[];
extern char GenPageDynHostRecEditReq[];
extern char GenPageDynDayTicketReq[];
extern char GenPageDynSLATicketReq[];
extern char GenPageCghGroupDbManage[];
extern char GenPageGroupDBManage[];
extern char GenPageChgGrpUser[];
extern char GenPageChgGrpSet[];
extern char GenPageCghProxyStatus[];
extern char GenPageProxyStatusReq[];
extern char GenPageProxyDeleteReq[];
extern char GenPageDynCghProxyStatus[];
extern char GenPageDynProxyEditReq[];
extern char GenPageProxyDBManage[];
extern char GenPageAlarmSortReq[];
extern char GenPageAlarmAnalyseReq[];
extern char GenPageAlarmToCsvExport[];
extern char GenPageCghAlarmServReqSent[];
extern char GenPagePowerCntrDBManage[];
extern char GenPageCghPowerCntrStatus[];
extern char GenPageDynCghPowerCntrStatus[];
extern char GenPagePowerCntrDeleteReq[];
extern char GenPagePowerCntrStatusReq[];
extern char GenPageDynPowerCntrEditReq[];
extern char GenPageDynActPowerStatsReq[];
extern char GenPagePowerCntrAnalyseReq[];
extern char GenPageOamServPowerCntrReq[];
extern char GenPageCghUsedPowerCntrReq[];
extern char GenPagePowerUsageToCsvExport[];
extern char GenPageDynHostDetailInfoReq[];
extern char GenPageDynAuthConfKeyReq[];
extern char GenPageDynEncodeDataReq[];
extern char GenPageDynHostCmdProc[];
extern char GenPageDynHostCmdInit[];
extern char GenPageDynDiscMgmt[];
extern char GenPageDynEthMgmt[];
extern char GenPageEthPortsStatusReq[];
extern char GenPageCghEthPortsStatus[];
extern char GenPageDynCghEthStatus[];
extern char GenPageDiscsStatusReq[];
extern char GenPageCghDiscsStatus[];
extern char GenPageDynCghDiscStatus[];
extern char GenPageDynHostDiscList[];
extern char GenPageDynHostCfgReq[];
extern char GenPageDynActionProcReq[];

extern char GenPageLogAnalyseReq[];
extern char GenPageOamServLogReq[];
extern char GenPageCghOamLogServReqSent[];
extern char AlarmFilterPathBaseName[];
extern char CsvTablePathBaseName[];

extern char UserDbNamePath[];
extern char HtmlDataPath[];
extern char SiteMapNamePath[];
extern char ItemShowCountNamePath[];
extern char UsersRequestHistNamePath[];
extern char CustomConfigNamePath[];
extern char MobileBeviceDbNamePath[];
extern char BackReqDbNamePath[];
extern char SiteMapStUrlDbNamePath[];
extern char LastSeeItemsDbNamePath[];
extern char SalesAnalyseNamePath[];
extern char VendorDbNamePath[];
extern char SoldItemsLotDbNamePath[];
extern char LogFilterPath[];
extern char FileNameMapDbNamePath[];
extern char StatsDbNamePath[];
extern char NoHostInfo[];
extern char BotInfoDbNamePath[];
extern char HostDbNamePath[];
extern char ActionDbNamePath[];
extern char ActionFileInfoPath[];
extern char RoomDbNamePath[] ;
extern char GroupDbNamePath[];
extern char PowerFilterPathBaseName[];
extern char UpgradePackagePath[];

extern char BtExitPageKey[];
extern char BtDetailedItemKey[];
extern char BtNavFirstPgDsKey[];
extern char BtNavFirstPgEnKey[];
extern char BtNavFirstPgMOKey[];
extern char BtNavFirstPgMSKey[];
extern char BtNavPrevPgDsKey[];
extern char BtNavPrevPgEnKey[];
extern char BtNavPrevPgMOKey[];
extern char BtNavPrevPgMSKey[];
extern char BtNavNextPgDsKey[];
extern char BtNavNextPgEnKey[];
extern char BtNavNextPgMOKey[];
extern char BtNavNextPgMSKey[];
extern char BtNavLastPgDsKey[];
extern char BtNavLastPgEnKey[];
extern char BtNavLastPgMOKey[];
extern char BtNavLastPgMSKey[];
extern char WorkZoneBgImageKey[];
extern char BtNavRetPgDsKey[];
extern char BtNavRetPgEnKey[];
extern char BtNavRetPgMOKey[];
extern char BtNavRetPgMSKey[];
extern char BtBacktInfoDsKey[];
extern char BtBacktInfoEnKey[];
extern char BtBacktInfoMoKey[];
extern char BtRemItemEnKey[];
extern char BtRemItemMoKey[];
extern char BtUserGrpEnKey[];
extern char BtUserGrpMoKey[];
extern char EmptyBlkKey[];
extern char BtInfSendServEnKey[];
extern char BtInfSendServMoKey[];
extern char BtFormCancelEnKey[];
extern char BtFormCancelMoKey[];
extern char BtFormCloseEnKey[];
extern char BtFormCloseMoKey[];
extern char BtExportExcelEnKey[];
extern char BtExportExcelMoKey[];
extern char BtExportExcelMsKey[];
extern char BtStopTestEnKey[];
extern char BtStopTestMoKey[];
extern char BtLogViewEnKey[];
extern char BtLogViewMoKey[];
extern char BtStatsViewEnKey[];
extern char BtStatsViewMoKey[];
extern char BtGetKeyEnKey[];
extern char BtGetKeyMoKey[];
extern char BtActionExecEnKey[];
extern char BtActionExecMoKey[];
extern char BtEnterEnKey[];
extern char BtEnterMoKey[];
extern char BtKeyEnKey[];
extern char BtKeyMoKey[];
extern char BtOkEnKey[];
extern char BtOkMoKey[];

extern char LogsGrpName[];
extern char NoHostInfo[];
//---------------------------------------------------------------------------
#endif  /* if ! defined( HtmlConstDataH ) */
