#ifndef __OPERATION_CODE__
#define __OPERATION_CODE__

#define S_SUCCESS                           1
#define S_LOGIN_HIGHSEC                     101
#define S_LOGIN_LOWSEC                      102
#define S_MODIFY_SECURITY_CODE              103
#define S_MODIFY_PASSWORD                   104

#define E_ERROR                             0
#define E_PDU_ERROR                         -1
#define E_PDU_LENGTH_ERROR                  -2
#define E_PDU_PARSE_ERROR                   -3
#define E_PDU_CHECKSUM_ERROR                -4
#define E_PDU_INVALID_PROTOCOL              -5
#define E_PDU_TIME_OUT                      -6
#define E_PDU_INVALID_FIELD                 -7
#define E_PDU_UNKNOWN_VERSION               -8
#define E_PARAMETER_ERROR                   -100
#define E_SYS_ERROR                         -200
#define E_SYS_NOT_ENOUGH_MEMORY             -201
#define E_SYS_TIMEOUT                       -202
#define E_SYS_THREAD_CREATE_ERROR           -210
#define E_SYS_NET_ERROR                     -300
#define E_SYS_NET_CONNECTION_ERROR          -301
#define E_SYS_NET_TIMEOUT                   -302
#define E_SYS_NET_IP_ERROR                  -303
#define E_SYS_NET_IP_BIND_ERROR             -304
#define E_SYS_NET_SERVER_START_ERROR        -305
#define E_SYS_NET_RECV_DATA_ERROR           -306
#define E_SYS_NET_SEND_DATA_ERROR           -307
#define E_SYS_NET_TOO_MANY_CONNECTION       -308
#define E_SYS_NET_NOT_ENOUGH_BUFFER         -309
#define E_SYS_NET_CLOSED                    -310
#define E_SYS_NET_INVALID                   -311
#define E_SYS_LICENSE_ERROR                 -400
#define E_SYS_LICENSE_OVERFLOW              -401
#define E_SYS_LICENSE_EXPIRED               -402
#define E_SYS_DATABASE_ERROR                -500
#define E_SYS_DATABASE_CONNECT_ERROR        -501
#define E_SYS_DATABASE_UIDPWD_ERROR         -502
#define E_SYS_DATABASE_TIMEOUT              -503
#define E_SYS_DATABASE_NO_CONNECTION        -504
#define E_SYS_CONFIG_ERROR                  -600
#define E_SYS_CONFIG_NO_ITEM                -601
#define E_SYS_FILE_NOT_FOUND                -700
#define E_SYS_FILE_RW_ERROR                 -701
#define E_SYS_FILE_DISK_FULL                -702
#define E_SYS_PATH_NOT_FOUND                -800
#define E_SYS_PATH_CREATE_ERROR             -801
#define E_SYS_PATH_CHANGE_ERROR             -802
#define E_SYS_PATH_RW_ERROR                 -803
#define E_GATEWAY_ERROR                     -1000
#define E_GATEWAY_NOT_FOUND                 -1001
#define E_GATEWAY_UID_PWD_ERROR             -1002
#define E_GATEWAY_IP_ERROR                  -1003
#define E_GATEWAY_MAC_ERROR                 -1004
#define E_GATEWAY_STATE_ERROR               -1010
#define E_GATEWAY_STATE_IS_ONLINE           -1011
#define E_GATEWAY_STATE_IS_OFFLINE          -1012
#define E_GATEWAY_STATE_IS_BUSY             -1013
#define E_ZONE_ERROR                        -1100
#define E_ZONE_NOT_FOUND                    -1101
#define E_ZONE_STATE_ERROR                  -1110
#define E_ACCOUNT_ERROR                     -1200
#define E_ACCOUNT_NOT_FOUND                 -1201
#define E_ACCOUNT_INVALID                   -1202
#define E_ACCOUNT_LENGTH_ERROR              -1203
#define E_ROLE_ERROR                        -1250
#define E_ROLE_EXIST                        -1251
#define E_ROLE_NOT_EXIST                    -1252
#define E_ROLE_DELETED                      -1253
#define E_USER_PASSWORD_ERROR               -1402
#define E_USER_SECURITYPW_ERROR             -1403
#define E_USER_EMAILCODE_ERROR              -1404
#define E_USER_NOEMAIL_ERROR                -1405
#define E_USER_ACCOUNT_ERROR                -1406
#define E_USER_REG_EXIST_ERROR              -1407
#define E_USER_USERINFO_ERROR               -1408
#define E_USER_ERROR                        -1400
#define E_USER_ACCOUNT_PASSWORD_ERROR       -1401
#define E_USER_STATE_ERROR                  -1410
#define E_USER_STATE_FREEZED                -1411
#define E_USER_STATE_EXPIRED                -1412
#define E_USER_STATE_NOT_ACTIVATED          -1413
#define E_USER_TYPE_ERROR                   -1420
#define E_USER_IN_OTHER_GATEWAY             -1430
#define E_USER_BALANCE_ERROR                -1440
#define E_USER_BALANCE_NOT_ENOUGH           -1441
#define E_USER_BALANCE_POINTS_NOT_ENOUGH    -1442
#define E_USER_BALANCE_DATE_EXPIRED         -1443
#define E_USER_BALANCE_COIN_NOT_ENOUGH      -1444
#define E_USER_IB_ITEM_ERROR                -1450
#define E_USER_IB_ITEM_NOT_FOUND            -1451
#define E_USER_IB_ITEM_TYPE_ERROR           -1452
#define E_USER_IB_ITEM_USE_TYPE_ERROR       -1453
#define E_USER_IB_ITEM_PRICE_ERROR          -1454
#define E_USER_IB_ITEM_DURATION_ERROR       -1455
#define E_USER_IB_ITEM_EXISTED              -1456
#define E_USER_IB_ITEM_NOT_ENOUGH           -1457
#define E_USER_IB_ITEM_USED                 -1458
#define E_USER_IB_ITEM_EXPIRED              -1459
#define E_USER_CHARGE_ERROR                 -1470
#define E_USER_CHARGE_STATE_ERROR           -1471
#define E_USER_SERIALNO_ERROR               -1480
#define E_USER_SERIALNO_NOT_FOUND           -1481
#define E_USER_SERIALNO_USED                -1482
#define E_USER_SERIALNO_EXPIRED             -1483
#define E_USER_SERIALNO_EXISTED             -1484

#endif
