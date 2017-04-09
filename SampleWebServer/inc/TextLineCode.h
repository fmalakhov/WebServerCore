# if ! defined( TextLineCodeH )
#	define TextLineCodeH /* only include me once */

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

#ifndef BaseTextLineCode
#include "BaseTextLineCode.h"
#endif

#define SITE_RUS_HEAD_2_LINE_ID        2
#define SITE_RUS_HEAD_3_LINE_ID        3
#define SITE_RUS_INFORMATION_LINE_ID   5
#define SITE_RUS_MAIN_INFO_LINE_ID     6
#define SITE_RUS_HOST_BASE_LINE_ID     7
#define SITE_RUS_HOST_DB_SYNC_BASE_LINE_ID 8

#define SITE_RUS_AUTH_REMEMBER_LINE_ID 12
#define SITE_RUS_AUTH_ENTER_LINE_ID    13

#define SITE_RUS_LOG_ANAL_REQ_LINE_ID  17
#define SITE_RUS_CLIENTSTATUS_CLIENTVER_HINT_LINE_ID 18
#define SITE_RUS_CLIENTSTATUS_CLIENTVER_SHD_LINE_ID  19

#define SITE_RUS_NOW_ON_SITE_LINE_ID   22
#define SITE_RUS_TODAY_DATE_LINE_ID    24

#define SITE_RUS_TEST_WAS_CHANGED_LINE_ID 28
#define SITE_RUS_AUTH_USER2_LINE_ID    29

#define SITE_RUS_REFFERENCE_LINE_ID    32
#define SITE_RUS_ROLE_STATUS_SHOW_LINE_ID  33
#define SITE_RUS_SECTION_NOT_FOUND_LINE_ID 34
#define SITE_RUS_LOG_TIME_INTERVAL_LINE_ID 35
#define SITE_RUS_SECT_VIEW_LINE_ID     36
#define SITE_RUS_SORT_BY_LINE_ID       37
#define SITE_RUS_SELECT_LINE_ID        38
#define SITE_RUS_NAMEIT_LINE_ID        39

#define SITE_RUS_SORT_BY_UP_LINE_ID    43
#define SITE_RUS_SORT_BY_DOWN_ID       44
#define SITE_RUS_SENT_REQ_LINE_ID      45

#define SITE_RUS_DETAIL_INFO_LINE_ID   50
#define SITE_RUS_SHOW_ITEMS_LINE_ID    51
#define SITE_RUS_RESULTS_LINE_ID       52
#define SITE_RUS_NUM_ITEMS_LINE_ID     53
#define SITE_RUS_ACTION_DB_FILE_NOT_SELECT_LINE_ID 54
#define SITE_RUS_TEST_IN_PROGR_STATUS_LINE_ID 55
#define SITE_RUS_CFG_SMS_SERV_ENCODE_LINE_ID  56

/* HTML reserv */
#define SITE_RUS_ITEM_DESCR_DATE_LINE_ID 61
#define SITE_RUS_ABOUT_SERVER_LINE_ID    62
/* 63 - html reserv */
#define SITE_RUS_RQ_HANDLE_TIME_LINE_ID  64
#define SITE_RUS_RUN_LOG_TYPE_FILTER_LINE_ID 65
#define SITE_RUS_REGEN_LINE_ID           70
#define SITE_RUS_CFG_MAIL_CLIENT_USE_LINE_ID 71
#define SITE_RUS_AVAIL_GROUP_LINE_ID         72
#define SITE_RUS_UFMI_SUBS_INF_LINE_ID   73
#define SITE_RUS_RETURN_BACK_LINE_ID     74
#define SITE_RUS_CONT_INFO_LINE_ID       75
#define SITE_RUS_SERV_ADMIN_INFO_LINE_ID 76
#define SITE_RUS_USER_REG_1_LINE_ID      77
#define SITE_RUS_USER_REG_2_LINE_ID      78
#define SITE_RUS_USER_REG_3_LINE_ID      79
#define SITE_RUS_USER_REG_4_LINE_ID      80
#define SITE_RUS_USER_REG_5_LINE_ID      81
#define SITE_RUS_USER_REG_6_LINE_ID      82
#define SITE_RUS_USER_REG_7_LINE_ID      83
#define SITE_RUS_USER_REG_8_LINE_ID      84
#define SITE_RUS_USER_REG_9_LINE_ID      85
#define SITE_RUS_USER_REG_10_LINE_ID     86
#define SITE_RUS_USER_REG_11_LINE_ID     87
#define SITE_RUS_USER_REG_12_LINE_ID     88
#define SITE_RUS_USER_REG_13_LINE_ID     89
#define SITE_RUS_AUTH_ERROR_LINE_ID      90

#define SITE_RUS_USER_ALREADY_EXIST_LINE_ID 92

#define SITE_RUS_ITEM_SEARCH_KEY_LINE_ID 94
#define SITE_RUS_USERS_SEARCH_LINE_ID    95

#define SITE_RUS_ABOUT_SHOP_LINE_ID      97

#define SITE_RUS_ORDER_1_LINE_ID         99
#define SITE_RUS_ORDER_USER_NAME_LINE_ID   100
#define SITE_RUS_ORDER_USER_PASSWD_LINE_ID 101
#define SITE_RUS_ORDER_MEMORY_LINE_ID      102
#define SITE_RUS_ORDER_2_LINE_ID           103
#define SITE_RUS_ORDER_3_LINE_ID           104
#define SITE_RUS_ORDER_4_LINE_ID           105

#define SITE_RUS_ORDER_6_LINE_ID           107
#define SITE_RUS_ORDER_COMP_NAME_LINE_ID   108
#define SITE_RUS_ORDER_REQ_TYPE_LINE_ID    109
#define SITE_RUS_ORDER_REQ_TYPE_1_LINE_ID  110
#define SITE_RUS_ORDER_REQ_TYPE_2_LINE_ID  111
#define SITE_RUS_ORDER_REQ_TYPE_3_LINE_ID  112
#define SITE_RUS_ORDER_REQ_TYPE_4_LINE_ID  113
#define SITE_RUS_ORDER_REQ_TYPE_5_LINE_ID  114
#define SITE_RUS_ORDER_NAME_LINE_ID        115
#define SITE_RUS_ORDER_MID_NAME_LINE_ID    116
#define SITE_RUS_ORDER_LAST_NAME_LINE_ID   117
#define SITE_RUS_ORDER_ADDR_1_LINE_ID      118
#define SITE_RUS_ORDER_ADDR_2_LINE_ID      119
#define SITE_RUS_ORDER_CITY_LINE_ID        120
#define SITE_RUS_ORDER_INDEX_LINE_ID       121
#define SITE_RUS_ORDER_COUNTRY_LINE_ID     122
#define SITE_RUS_ORDER_RUS_LINE_ID         123
#define SITE_RUS_ORDER_MOB_PHONE_LINE_ID   124
#define SITE_RUS_ORDER_COND_LINE_ID        125
#define SITE_RUS_ORDER_AGREE_LINE_ID       126
#define SITE_RUS_ORDER_MUST_LINE_ID        127
#define SITE_RUS_PSTN_LINE_ID              128
#define SITE_RUS_FAX_LINE_ID               129
#define SITE_RUS_MY_CONTACTS_LINE_ID       130
#define SITE_RUS_MY_ORDERS_LINE_ID         131
#define SITE_RUS_CONTACT_INFO_CHG_LINE_ID  132
#define SITE_RUS_OTDER_NUM_LINE_ID         133
#define SITE_RUS_OTDER_STATUS_LINE_ID      134
#define SITE_RUS_OTDER_NUM_ITEMS_LINE_ID   135
#define SITE_RUS_CFG_SMS_PROXY_EN_LINE_ID  136
#define SITE_RUS_OTDER_DATE_LINE_ID        137
#define SITE_RUS_CFG_SMS_PROXY_ADDR_LINE_ID 138
#define SITE_RUS_ORDER_STATUS_1_LINE_ID    139
#define SITE_RUS_CFG_SMS_PROXY_PORT_LINE_ID 140
#define SITE_RUS_ORDER_STATUS_4_LINE_ID    142
#define SITE_RUS_DWLD_LOGS_LINE_ID         143
#define SITE_RUS_NO_LOGS_DWLD_LINE_ID      144
#define SITE_RUS_LDWL_LFN_FILD_LINE_ID     145
#define SITE_RUS_LDWL_LFN_HINT_LINE_ID     146
#define SITE_RUS_LDWL_NAME_FILD_LINE_ID    147
#define SITE_RUS_ROLE_ADMIN_LINE_ID        148
#define SITE_RUS_NEXT_STATUS_LINE_ID       149

#define SITE_RUS_NO_ENTER_CAPCHA_LINE_ID   151
#define SITE_RUS_SEND_SMS_ALARM_LINE_ID  152
#define SITE_RUS_SEND_MAIL_ALARM_LINE_ID 153

#define SITE_RUS_LDWL_NAME_HINT_LINE_ID         158
#define SITE_RUS_LDWL_SIZE_FILD_LINE_ID         159

#define SITE_RUS_LDWL_SIZE_HINT_LINE_ID         161
#define SITE_RUS_LDWL_ACT_FIELD_LINE_ID         162
#define SITE_RUS_LDWL_ACT_HINT_LINE_ID          163
#define SITE_RUS_LDWL_START_DWLD_LINE_ID        164

#define SITE_RUS_LMS_LINK_LINE_ID               172
#define SITE_RUS_SHOP_DB_REG_CLIENTS_LINE_ID    173
#define SITE_RUS_USERS_LIST_EMPTY_LINE_ID       174
#define SITE_RUS_USERS_LIST_CLIENT_LINE_ID      175
#define SITE_RUS_ORDER_INFO_1_LINE_ID           176
#define SITE_RUS_LOG_DATE_FROM_INF_LINE_ID      177
#define SITE_RUS_LOG_DATE_TO_INF_LINE_ID        178

#define SITE_RUS_CLIENT_IDENTITY_INF_LINE_ID    182
#define SITE_RUS_CLIENT_TYPE_INF_LINE_ID        183
#define SITE_RUS_CLIENT_INDEX_INF_LINE_ID       184

#define SITE_RUS_SHOP_SERVER_CONFIG_LINE_ID     192
#define SITE_RUS_MAIL_SERV_CFG_LINE_ID          193
#define SITE_RUS_CFG_SMTP_SERV_NAME_LINE_ID     194
#define SITE_RUS_CFG_SMTP_PORT_NUM_LINE_ID      195
#define SITE_RUS_CFG_SMTP_TIMEOUT_LINE_ID       196
#define SITE_RUS_CFG_MAIL_INT_LINE_ID           197
#define SITE_RUS_CFG_SHOP_MAIL_LINE_ID          198
#define SITE_RUS_CFG_SMTP_LOGIN_LINE_ID         199
#define SITE_RUS_CFG_SMTP_PASSWD_NUM_LINE_ID    200
#define SITE_RUS_PASSWD_REST_INSTR_1_LINE_ID    201
#define SITE_RUS_PASSWD_REST_INSTR_2_LINE_ID    202
#define SITE_RUS_PASSWD_REST_EMAIL_LINE_ID      203
#define SITE_RUS_PASSWD_REST_SENT_PASS_LINE_ID  204

#define SITE_RUS_FIELD_FILL_REQ_LINE_ID         210

#define SITE_RUS_FILTER_CLIENTID_HINT_LINE_ID   213
#define SITE_RUS_FILTER_IMEI_HINT_LINE_ID       214
#define SITE_RUS_FILTER_DATEFROM_HINT_LINE_ID   215
#define SITE_RUS_FILTER_DATETO_HINT_LINE_ID     216
#define SITE_RUS_FILTER_UFMI_HINT_LINE_ID       217

#define SITE_RUS_ROLE_LABENGR_LINE_ID           224
#define SITE_RUS_ROLE_MANAGER_LINE_ID           225
#define SITE_RUS_USER_TYPE_LINE_ID              226
#define SITE_RUS_ROLE_SITE_ADMIN_LINE_ID        227
#define SITE_RUS_ORDER_ADDR_3_LINE_ID           230
#define SITE_RUS_CUSTOM_UPDATE_LINE_ID          231
#define SITE_RUS_SYS_STATUS_LINE_ID             232

#define SITE_RUS_SALE_ST_HINT_LINE_ID           245
#define SITE_RUS_LOG_MANAGER_LINE_ID            246

#define SITE_RUS_ACTIVE_SESSIONS_LINE_ID        249
#define SITE_RUS_SESSIONS_INFO_CODE_LINE_ID     250
#define SITE_RUS_SESSIONS_IP_ADDR_LINE_ID       251
#define SITE_RUS_SESSIONS_DURAT_LINE_ID         252
#define SITE_RUS_SESSIONS_ANON_USR_LINE_ID      253

#define SITE_RUS_USERS_REGISTER_LINE_ID         260
#define SITE_RUS_USERS_LAST_VISIT_LINE_ID       261
#define SITE_RUS_ADM_PWD_CHG_LINE_ID            264
#define SITE_RUS_SUCC_ADM_PWD_CHG_LINE_ID       265
#define SITE_RUS_ADM_PWD_CHG_SEL_LINE_ID        266
#define SITE_RUS_SHOP_DB_CLONE_LINE_ID          267

#define SITE_RUS_CLONE_SHOW_NO_LINE_ID          322
#define SITE_RUS_CLONE_SHOW_YES_LINE_ID         323
#define SITE_RUS_HOST_DB_HOST_ROOM_LINE_ID      324
#define SITE_RUS_ROOM_NOT_ASSIGN_LINE_ID        325
#define SITE_RUS_NEW_USER_INFO_TITLE_LINE_ID    328
#define SITE_RUS_NEW_USER_INFO_1_LINE_ID        329
#define SITE_RUS_NEW_USER_INFO_2_LINE_ID        330
#define SITE_RUS_NEW_USER_INFO_3_LINE_ID        331
#define SITE_RUS_NEW_USER_INFO_4_LINE_ID        332
#define SITE_RUS_NEW_USER_INFO_5_LINE_ID        333
#define SITE_RUS_NEW_USER_INFO_6_LINE_ID        334
#define SITE_RUS_NEW_USER_INFO_7_LINE_ID        335

#define SITE_RUS_SORT_TYPE_HOSTID_LINE_ID       345
#define SITE_RUS_SORT_TYPE_TRANSTIME_LINE_ID    346
#define SITE_RUS_SORT_TYPE_CPU_LINE_ID          347
#define SITE_RUS_SORT_TYPE_MEM_LINE_ID          348
#define SITE_RUS_CLIENT_LIST_IP_HIDE_LINE_ID    349

#define SITE_RUS_SHOP_DB_OPER_CANCEL_LINE_ID    355
#define SITE_RUS_STATS_DB_GROUP_PERCENT2_LINE_ID 356

#define SITE_RUS_GENE_SERV_CFG_LINE_ID          357
#define SITE_RUS_CFG_GEN_MAX_SESS_LINE_ID       358
#define SITE_RUS_CFG_GEN_MAX_IP_SESS_LINE_ID    359
#define SITE_RUS_CFG_GEN_COMPR_EN_LINE_ID       360
#define SITE_RUS_CFG_GEN_MIN_PG_COMP_SZ_LINE_ID 361
#define SITE_RUS_CFG_GEN_KEEP_AL_EN_LINE_ID     362
#define SITE_RUS_CFG_GEN_KEEP_AL_TIME_LINE_ID   363
#define SITE_RUS_SERV_SHUT_RES_LINE_ID          364
#define SITE_RUS_SERV_SHUT_SUCC_LINE_ID         365
#define SITE_RUS_USER_DEL_REQ_LINE_ID              366
#define SITE_RUS_WEB_SERVER_SHUTDOWN_LINE_ID       367
#define SITE_RUS_SERVER_SHUTDOWN_START_INF_LINE_ID 368
#define SITE_RUS_HOST_DB_HOST_GROUP_LINE_ID        369
#define SITE_RUS_SET_COMMON_GROUP_LINE_ID          370
#define SITE_RUS_GROUP_BASE_LINE_ID                371
#define SITE_RUS_GROUP_DB_EMPTY_SEC_LINE_ID        372
#define SITE_RUS_GROUP_DFN_INDEX_LINE_ID           373
#define SITE_RUS_GROUP_DB_GROUP_NAME_LINE_ID       374
#define SITE_RUS_GROUP_DB_GROUP_EDIT_LINE_ID       375
#define SITE_RUS_GROUP_DB_GROUP_DELETE_LINE_ID     376
#define SITE_RUS_GROUP_DB_NEW_GROUP_LINE_ID        377
#define SITE_RUS_USER_GRP_SET_REQ_LINE_ID          378
#define SITE_RUS_EDIT_USER_HOST_ACC_GRP_LINE_ID    379
#define SITE_RUS_EDIT_GRP_USER_INFO_LINE_ID        380
#define SITE_RUS_HOST_DB_GROUP_LINE_ID             381
#define SITE_RUS_CAPCHA_INTRO_LINE_ID              382
#define SITE_RUS_GENINFO_DEMO_MODE_LINE_ID         383

#define SITE_RUS_WRONG_CONF_KEY_INFO_LINE_ID    455

#define SITE_RUS_CHG_ITEM_DATE_INFO_LINE_ID     457
#define SITE_RUS_INT_LIVE_INFO_1_LINE_ID        458
#define SITE_RUS_INT_LIVE_INFO_2_LINE_ID        459
#define SITE_RUS_RATING_MAIL_RU_LINE_ID         460
#define SITE_RUS_LEFT_CHAR_ENTER_1_LINE_ID      461
#define SITE_RUS_LEFT_CHAR_ENTER_2_LINE_ID      462

#define SITE_RUS_RESP_SHOP_MSG_LINE_ID          469
#define SITE_RUS_FILE_HASH_UPDATE_LINE_ID       470

#define SITE_RUS_SHOP_INFO_CFG_LINE_ID          505

#define SITE_RUS_CFG_SHOP_LATITUDE_LINE_ID      520
#define SITE_RUS_CFG_SHOP_LONGITUDE_LINE_ID     521

#define SITE_RUS_NEW_USER_INFO_8_LINE_ID        524

#define SITE_RUS_CFG_GRP_NAME_GENERAL_LINE_ID   527
#define SITE_RUS_CFG_GRP_NAME_MAIL_LINE_ID      528
#define SITE_RUS_CFG_GRP_NAME_SHOP_LINE_ID      529
#define SITE_RUS_CFG_GRP_NAME_BANNER_LINE_ID    530
#define SITE_RUS_BANNER_CFG_LINE_ID             531
#define SITE_RUS_NEW_BANNER_SET_LINE_ID         532
#define SITE_RUS_BANNER_LIST_EMPTY_LINE_ID      533
#define SITE_RUS_BANNER_LOCATION_LINE_ID        534
#define SITE_RUS_BANNER_NAME_LINE_ID            535
#define SITE_RUS_CFG_BLOC_HIDE_LINE_ID          536
#define SITE_RUS_CFG_BLOC_PGDWN_LINE_ID         537
#define SITE_RUS_CFG_BLOC_PGRIGHT_LINE_ID       538
#define SITE_RUS_BANNER_EDIT_LINE_ID            539
#define SITE_RUS_BANNER_SAVE_LINE_ID            540
#define SITE_RUS_BANNER_BODY_LINE_ID            541
#define SITE_RUS_BANNER_DELETE_LINE_ID          542
#define SITE_RUS_BANNER_ACTIVE_LINE_ID          543
#define SITE_RUS_BANNER_ACTIVE_STATUS_LINE_ID   544
#define SITE_RUS_BANNER_STATUS_INF_LINE_ID      545
/* 546 - 549 Reserv for SMS client */
#define SITE_RUS_CFG_GRP_NAME_SMS_LINE_ID       550
#define SITE_RUS_SMS_CFG_LINE_ID                551
#define SITE_RUS_NEW_PHONE_SET_LINE_ID          552
#define SITE_RUS_CFG_SMS_SERV_NAME_LINE_ID      553
#define SITE_RUS_CFG_SMS_PORT_NUM_LINE_ID       554
#define SITE_RUS_CFG_SMS_TIMEOUT_LINE_ID        555
#define SITE_RUS_SMS_PHONE_LIST_EMPTY_LINE_ID   556
#define SITE_RUS_SMS_TEST_RUS_LINE_ID           557
#define SITE_RUS_SMS_RUS_LANG_SEL_LINE_ID       558
#define SITE_RUS_SMS_PHONE_EDIT_LINE_ID         559
#define SITE_RUS_SMS_PHONE_NUM_LINE_ID          560
#define SITE_RUS_SMS_PHONE_SAVE_LINE_ID         561
#define SITE_RUS_SMS_LANG_INFO_LINE_ID          562
#define SITE_RUS_SMS_NUM_DELETE_LINE_ID         563
#define SITE_RUS_CFG_SMS_ACCESS_ID_LINE_ID      564
#define SITE_RUS_CFG_SMS_SRC_LINE_ID            565
#define SITE_RUS_SMS_RUS_LINE_ID                566

/*********** User's Text IDs should be started from ID: 600 ********/
#define SITE_RUS_DEMO_OVERVIEW_LINE_ID			600

//---------------------------------------------------------------------------
#endif  /* if ! defined( TextLineCodeH ) */
