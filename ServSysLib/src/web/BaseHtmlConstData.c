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

#include "CommonPlatform.h"
#include "vistypes.h"
#include "ImageNameHash.h"

char GenPageMain[] = "Main.html";
char GenPageUserAuthReq[] = "UserAuthReq.html";
char GenPageLostPasswd[] = "lostPasswd.html";
char GenPageUserRegister[] = "userRegister.html";
char GenPageUserExitReq[] = "UserExitReq.html";
char GenPageLangSelect[] = "LangSelectReq.html";
char GenPageBadIpDBManage[] = "BadIpDBManage.html";
char GenPageSpaseResize[] = "SpaseResizeReq.html";
char GenPageContacts[] = "Contacts.html";
char GenPageDynStatusShowSessIdReq[] = "StatusShowSessionIdReq.htr";
char GenPageDynConfKeyReq[] = "AccessConfKeyReq.htr";
char GenPageDynServDateTime[] = "ServDateTimeReq.htr";

char InputLineStyle[] = "style=\"float:left;width:30%;text-align:right;vertical-align:bottom;font-weight: bold;padding-right: 5px;\"";
char NoPhotoPicture[]    = "images/no_photo.jpg";
char CapchaFileIdName[]  = "user_check_capcha.png";

char StatsNameExt[]     = ".scd";
char StatsHistNameExt[] = ".sci";
char ServerConfigExt[]  = ".cfg";

#ifdef WIN32
char HtmlTemplateDir[]   = "\\html_template\\";
char BaseWebDataPath[]   = "\\WebData\\";
char BaseWebDataPath2[   = "\\WebData";
char HtmlDataPath[]      = "\\WebData\\html_data\\";
char TestFileInfoPath[]  = "\\test_data\\";
char RusTextDbNamePath[] = "\\database\\rus_text_base.db";
char EngTextDbNamePath[] = "\\database\\eng_text_base.db";
char UserDbNamePath[]    = "\\database\\user_info.db";
char BadIpDbNamePath[]   = "\\database\\bad_ip_list.db";
char SiteMapNamePath[]   = "\\sitemap.xml";
char CustomConfigNamePath[]     = "\\database\\custom_config.db";
char MobileBeviceDbNamePath[]   = "\\database\\mobile_device.db";
char SiteMapStUrlDbNamePath[]   = "\\database\\site_map_static_url.db";
char FileNameMapDbNamePath[]    = "\\database\\file_map_base.db";
char StatsDbNamePath[]          = "\\database\\stats_base.db";
char BotInfoDbNamePath[]        = "\\database\\bot_base.db";
char GroupDbNamePath[]          = "\\database\\group_base.db";
char LogFilterPath[]            = "\\WebData\\file_gen";
char ActionFileInfoPath[]       = "\\action_data\\"
#else
char HtmlTemplateDir[]   = "/html_template/";
char BaseWebDataPath[]   = "/WebData/";
char BaseWebDataPath2[]  = "/WebData";
char HtmlDataPath[]      = "/WebData/html_data/";
char RusTextDbNamePath[] = "/database/rus_text_base.db";
char EngTextDbNamePath[] = "/database/eng_text_base.db";
char UserDbNamePath[]    = "/database/user_info.db";
char BadIpDbNamePath[]   = "/database/bad_ip_list.db";
char SiteMapNamePath[]   = "/sitemap.xml";
char CustomConfigNamePath[]     = "/database/custom_config.db";
char MobileBeviceDbNamePath[]   = "/database/mobile_device.db";
char SiteMapStUrlDbNamePath[]   = "/database/site_map_static_url.db";
char TestDbNamePath[]           = "/database/test_base.db";
char FileNameMapDbNamePath[]    = "/database/file_map_base.db";
char StatsDbNamePath[]          = "/database/stats_base.db";
char BotInfoDbNamePath[]        = "/database/bot_base.db";
char GroupDbNamePath[]          = "/database/group_base.db";
char LogFilterPath[]            = "/WebData/file_gen";
#endif

char KeyRetTargetId[]     = "rettarget_id";
char KeyUserSearchLine[]  = "user_search_line";
char LogFilterPathBaseName[] = "log_filter_";
char StatsFilterPathBaseName[] = "stats_filter_";
char LogFilterExt[]          = ".csv";

char KeyDateFromId[]      = "date_from";
char KeyDateToId[]        = "date_to";
char KeyFormClientType[]  = "client_type";
char KeyClientId[]        = "client_id";
char KeyFormCapchaCode[]  = "capcha_code";
char KeyFormConfirmKey[]  = "confirm_key";
char KeyDeviceId[]        = "device_id";
char KeyFormScreenWidth[] = "screen_width";
char KeyFormScreenHeight[] = "screen_height";

//---------------------------------------------------------------------------
