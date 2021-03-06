
//#include "main.h"
#include "lwip/debug.h"
#include "httpd.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
//#include "app_ethernet.h"
#include "fs.h"
#include "ustdlib.h"
#include "config.h"
#include "Control/W25X32.h"
#include "Sntp/sntp.h"
#include "Control/control.h"

//#include "W25X32.h"
//#include "flash_pb.h"
//#include "PCF8563.h"
//#include "serial.h"
//#include "telnet.h"
//#include "TIMER3.h"
//#include "keepalive.h"
#include <string.h>
#include <stdlib.h>
#define FLASH_CONFIG_SIZE sizeof(tConfigParameters)
#define FLASH_SYCCAL_SIZE sizeof(tSysCALParameters)
#define CACHE_BLOCK_SIZE  4096

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! snmp varirs
unsigned char HostAddr[4];
unsigned char MACAddress[6];

//*****************************************************************************
//RTC_T    PCF8563RTC_PROOFTIME;
//*****************************************************************************
//
//! A local flag indicating that a firmware update has been requested via the
//! web-based configuration pages.
//
//*****************************************************************************
static tBoolean g_bUpdateRequested = false;

//*****************************************************************************
//
//! A flag to the main loop indicating that it should enter the bootloader and
//! perform a firmware update.
//
//*****************************************************************************
tBoolean g_bStartBootloader = false;

//*****************************************************************************
//
//! A flag to the main loop indicating that it should update the IP address
//! after a short delay (to allow us to send a suitable page back to the web
//! browser telling it the address has changed).
//
//*****************************************************************************
tBoolean g_bChangeIPAddress = false;
tBoolean bReset = false;
tBoolean ugSntp_Changed = false;

//*****************************************************************************
//
//! The maximum length of any HTML form variable name used in this application.
//
//*****************************************************************************
#define MAX_VARIABLE_NAME_LEN   16

//*****************************************************************************
//
// SSI tag indices for each entry in the g_pcSSITags array.
//
//*****************************************************************************
#define SSI_INDEX_CTRVARS       0
#define SSI_INDEX_STATUS1       1
#define SSI_INDEX_STATUS2       2
#define SSI_INDEX_STATUS3       3
#define SSI_INDEX_STATUS4       4
#define SSI_INDEX_LAMP1         5
#define SSI_INDEX_LAMP2         6
#define SSI_INDEX_LAMP3         7
#define SSI_INDEX_LAMP4         8

#define SSI_INDEX_UNEARTH       9
#define SSI_INDEX_LNORDER       10
#define SSI_INDEX_LPROFAIL      11
#define SSI_INDEX_OVCURRA       12
#define SSI_INDEX_OVVOLA        13
#define SSI_INDEX_UVLOA         14
#define SSI_INDEX_LEAKCA    15
#define SSI_INDEX_INVCH1        16
#define SSI_INDEX_INVCH2        17
#define SSI_INDEX_INVCH3        18
#define SSI_INDEX_INVCH4        19

#define SSI_INDEX_OVERVOL       20
#define SSI_INDEX_UVLO          21
#define SSI_INDEX_OVERCUR       22
#define SSI_INDEX_LEAKCUR       23

#define SSI_INDEX_MACVARS       24
#define SSI_INDEX_IPVARS        25
#define SSI_INDEX_SNVARS        26
#define SSI_INDEX_GWVARS        27
#define SSI_INDEX_DNSVAR        28
#define SSI_INDEX_WEBPORT       29
#define SSI_INDEX_PNPINP        30
#define SSI_INDEX_NOPID         31
#define SSI_INDEX_OLDPID        32

#define SSI_INDEX_SNTPVARS      33
#define SSI_INDEX_SNTPURL       34
#define SSI_INDEX_SNTPINTER     35

#define SSI_INDEX_TRAPVARS      36
#define SSI_INDEX_TRAPIP        37
#define SSI_INDEX_TRAPPORT      38

#define SSI_INDEX_SWENVARS      39
#define SSI_INDEX_BOOTHVARS     40
#define SSI_INDEX_BOOTMVARS     41
#define SSI_INDEX_SHUTHVARS     42
#define SSI_INDEX_SHUTMVARS     43

#define SSI_INDEX_RGVARS        44
#define SSI_INDEX_DELAY1        45
#define SSI_INDEX_DELAY2        46
#define SSI_INDEX_DELAY3        47

#define SSI_INDEX_MODENAME      48
#define SSI_INDEX_MODEL         49
#define SSI_INDEX_MANUFACT      50
#define SSI_INDEX_SERIAL        51
#define SSI_INDEX_POSITION      52
#define SSI_INDEX_INSTALPERSON       53
#define SSI_INDEX_INSTALTIME         54
#define SSI_INDEX_RUNTIME            55
#define SSI_INDEX_SOFTVER            56
#define SSI_INDEX_HARDVER            57
#define SSI_INDEX_CURTIME            58

#define SSI_INDEX_SYSVOL             59
#define SSI_INDEX_SYSCUR             60
#define SSI_INDEX_LAMP               61
#define SSI_INDEX_TEMPVAR              62
#define SSI_INDEX_HUMIVAR              63

//! XML Temp Buffer Size
#define XML_TMPBUF_SIZE             96
//
//Globle Vars
//

tTimeParameters  CurrentTime;
/*Channel Line on-off status*/
unsigned char  ChannelStatus;
/*Channel Line on-off  indicate lamp status*/
unsigned char  ChannelLamp = 0x0f;
/*system alarm recond flag*/
unsigned short SysAlarm_Flag = 0;

//Private Vars
static const unsigned short imark[11] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400};
//static old run day
static unsigned long old_SysRunDay = 0;
//*****************************************************************************
//
// The address of the most recent parameter block in flash.
//
//*****************************************************************************
static unsigned char* g_pucFlashPBCurrent;

//! analog value.
extern u16_t analogValue[3];  //0: AC VOL 1:LEAKAGE CURRENT. 2: LOAD CURRENT.
extern s16_t discreteValue[8];
extern int temp_value;
extern int humi_value;
//! quene name
typedef enum
{
    XML_SYSVOL_STRING,//XML System AC Voltage String  Quene
    XML_SYSCUR_STRING,//XML System AC Load Current String Quene
    XML_SYSTMP_STRING,//XML System Temperature String Quene
    XML_SYSHUI_STRING,//XML System Humidity String Quene
    XML_CH1STA_STRING,//XML Fist Channel State String Quene
    XML_CH2STA_STRING,//XML Second Channel State String Quene
    XML_CH3STA_STRING,//XML Third Channel State String Quene
    XML_CH4STA_STRING,//XML Fourth Channel State String Quene
    XML_CH1RSN_STRING,//XML Fist Channel SW Active Reason String Quene
    XML_CH2RSN_STRING,//XML Second Channel SW Active Reason String Quene
    XML_CH3RSN_STRING,//XML Third Channel SW Active Reason String Quene
    XML_CH4RSN_STRING //XML Fourth Channel SW Active Reason String Quene
} XML_STRING_NAME;
//*****************************************************************************
//
//! This array holds all the strings that are to be recognized as SSI tag
//! names by the HTTPD server.  The server will call ConfigSSIHandler to
//! request a replacement string whenever the pattern <!--#tagname--> (where
//! tagname appears in the following array) is found in ``.ssi'' or ``.shtm''
//! files that it serves.
//
//*****************************************************************************
static const char* g_pcConfigSSITags[] =
{
    "ctrvars",    // SSI_INDEX_CTRVARS
    "st1",        // SSI_INDEX_STATUS1
    "st2",        // SSI_INDEX_STATUS2
    "st3",        // SSI_INDEX_STATUS3
    "st4",        // SSI_INDEX_STATUS4
    "lamp1",      // SSI_INDEX_LAMP1
    "lamp2",      // SSI_INDEX_LAMP2
    "lamp3",      // SSI_INDEX_LAMP3
    "lamp4",      // SSI_INDEX_LAMP4
    "uearth",     // SSI_INDEX_UNEARTH
    "uorder",     // SSI_INDEX_LNORDER
    "profail",    // SSI_INDEX_LPROFAIL
    "oca",        // SSI_INDEX_OVCURRA
    "ova",    // SSI_INDEX_OVVOLA
    "uvloa",      // SSI_INDEX_UVLOA
    "leakca",     // SSI_INDEX_LEAKCA
    "invch1",     // SSI_INDEX_INVCH1
    "invch2",     // SSI_INDEX_INVCH2
    "invch3",     // SSI_INDEX_INVCH3
    "invch4",     // SSI_INDEX_INVCH4
    "ov",         // SSI_INDEX_OVERVOL
    "uvlo",       // SSI_INDEX_UVLO
    "oc",         // SSI_INDEX_OVERCUR
    "lc",         // SSI_INDEX_LEAKCUR
    "macvars",    // SSI_INDEX_MACVARS
    "ipvars",     // SSI_INDEX_IPVARS
    "snvars",     // SSI_INDEX_SNVARS
    "gwvars",     // SSI_INDEX_GWVARS
    "dnsvars",    // SSI_INDEX_DNSVAR
    "webprt",     // SSI_INDEX_WEBPORT
    "pnpinp",     // SSI_INDEX_PNPINP
    "nopid",      // SSI_INDEX_NOPID
    "oldpid",     // SSI_INDEX_OLDPID
    "ntpvars",    // SSI_INDEX_SNTPVARS
    "spurl",      // SSI_INDEX_SNTPURL
    "spinter",    // SSI_INDEX_SNTPINTER
    "tpvars",     // SSI_INDEX_TRAPVARS
    "trip",       // SSI_INDEX_TRAPIP
    "trpt",       // SSI_INDEX_TRAPPORT
    "swvars",     // SSI_INDEX_SWENVARS
    "bthvars",    // SSI_INDEX_BOOTHVARS
    "btmvars",    // SSI_INDEX_BOOTMVARS
    "sthvars",    // SSI_INDEX_SHUTHVARS
    "stmvars",    // SSI_INDEX_SHUTMVARS
    "rgvars",     // SSI_INDEX_RGVARS
    "delay1",     // SSI_INDEX_DELAY1
    "delay2",     // SSI_INDEX_DELAY2
    "delay3",     // SSI_INDEX_DELAY3
    "modename",   // SSI_INDEX_MODENAME
    "model",      // SSI_INDEX_MODEL
    "manu",       // SSI_INDEX_MANUFACT
    "serial",     // SSI_INDEX_SERIAL
    "pos",        // SSI_INDEX_POSITION
    "instal",     // SSI_INDEX_INSTALPERSON
    "instime",    // SSI_INDEX_INSTALTIME
    "runtime",    // SSI_INDEX_RUNTIME
    "sofver",     // SSI_INDEX_SOFTVER
    "harver",     // SSI_INDEX_HARDVER
    "currt",      // SSI_INDEX_CURTIME
    "sysvol",     // SSI_INDEX_SYSVOL
    "syscur",     // SSI_INDEX_SYSCUR
    "lamp",       // SSI_INDEX_LAMP
    "tempvar",    // SSI_INDEX_TEMPVAR
    "humivar"     // SSI_INDEX_HUMIVAR
};

//*****************************************************************************
//
//! The number of individual SSI tags that the HTTPD server can expect to
//! find in our configuration pages.
//
//*****************************************************************************
#define NUM_CONFIG_SSI_TAGS     (sizeof(g_pcConfigSSITags) / sizeof (char *))


//*****************************************************************************
//
//! Prototype for the function which handles requests for serial.cgi.
//
//*****************************************************************************
static const char*
ConfigHomeHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] );
//*****************************************************************************
//
//! Prototype for the function which handles requests for config.cgi.
//
//*****************************************************************************
static const char* SNTPCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                                   char* pcValue[] );
//*****************************************************************************
//
//! Prototype for the function which handles requests for config1.cgi.
//
//*****************************************************************************
static const char* Config2CGIHandler1( int iIndex, int iNumParams, char* pcParam[],
                                       char* pcValue[] );

//*****************************************************************************
//
//! Prototype for the function which handles requests for misc.cgi.
//
//*****************************************************************************
static const char* ConfigTrapCGIHandler( int iIndex, int iNumParams, char* pcParam[],
        char* pcValue[] );

//*****************************************************************************
//
//! Prototype for the function which handles requests for ip.cgi.
//
//*****************************************************************************
static const char* ConfigIPCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                                       char* pcValue[] );

//*****************************************************************************
//
//! Prototype for the function which handles requests for update.cgi.
//
//*****************************************************************************
static const char* ConfigSwitchCGIHandler( int iIndex, int iNumParams,
        char* pcParam[], char* pcValue[] );

//*****************************************************************************
//
//! Prototype for the function which handles requests for defaults.cgi.
//
//*****************************************************************************
static const char* ConfigRegateCGIHandler( int iIndex, int iNumParams,
        char* pcParam[], char* pcValue[] );

static const char* LoginCGIHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] );
static const char* ConfigsetpwdCGIHandler( int iIndex, int iNumParams, char* pcParam[],
        char* pcValue[] );
static const char*
ConfigCTRStyeCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                         char* pcValue[] );
static const char*
ConfigInfoCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                      char* pcValue[] );
static const char*
ConfigresetCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                       char* pcValue[] );

static const char*
Orther_CGIHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] );
static const char*
Opther_CGIHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] );
//*****************************************************************************
//
//! Prototype for the main handler used to process server-side-includes for the
//! application's web-based configuration screens.
//
//*****************************************************************************
static uint16_t ConfigSSIHandler( int iIndex, char* pcInsert, int iInsertLen );



//*****************************************************************************
//
// CGI URI indices for each entry in the g_psConfigCGIURIs array.
//
//*****************************************************************************
#define CGI_INDEX_CONFIG        0
#define CGI_INDEX_MISC          1
#define CGI_INDEX_UPDATE        2
#define CGI_INDEX_DEFAULTS      3
#define CGI_INDEX_IP            4

//*****************************************************************************
//
//! This array is passed to the HTTPD server to inform it of special URIs
//! that are treated as common gateway interface (CGI) scripts.  Each URI name
//! is defined along with a pointer to the function which is to be called to
//! process it.
//
//*****************************************************************************
static const tCGI g_psConfigCGIURIs[] =
{
    { "/login.cgi", LoginCGIHandler },              // CGI_INDEX_Login
    { "/contrl.cgi", ConfigHomeHandler },           // CGI_INDEX_CONFIG
    { "/thresh.cgi", Config2CGIHandler1 },            // CGI_INDEX_CONFIG
    { "/ip.cgi", ConfigIPCGIHandler },              // CGI_INDEX_IP
    { "/sntp.cgi", SNTPCGIHandler },            // CGI_INDEX_CONFIG
    { "/trap.cgi", ConfigTrapCGIHandler },          // CGI_INDEX_MISC
    { "/sw.cgi", ConfigSwitchCGIHandler },      // CGI_INDEX_UPDATE
    { "/regate.cgi", ConfigRegateCGIHandler },  // CGI_INDEX_DEFAULTS
    { "/reset.cgi", ConfigresetCGIHandler },
    { "/setpwd.cgi", ConfigsetpwdCGIHandler },
    { "/ctrstye.cgi", ConfigCTRStyeCGIHandler },
    { "/info.cgi", ConfigInfoCGIHandler },
    { "/orther.cgi",  Orther_CGIHandler },
    { "/opther.cgi",  Opther_CGIHandler }
};

//*****************************************************************************
//
//! The number of individual CGI URIs that are used by our configuration
//! web pages.
//
//*****************************************************************************
#define NUM_CONFIG_CGI_URIS     (sizeof(g_psConfigCGIURIs) / sizeof(tCGI))

//*****************************************************************************
//
//! The file sent back to the browser by default following completion of any
//! of our CGI handlers.
//
//*****************************************************************************
#define LOGIN_CGI_RESPONE       "/index.html"
#define LOGIN_CGI_RESPONE1      "/index1.html"
#define PASSWD_CGI_RESPONE      "/passwd.shtml"
#define CONFIG2_CGI_RESPONE     "/config2.shtml"
#define DEFAULT_CGI_RESPONSE    "/home.shtml"
#define DEFAULT_CGI_RESPONSE1    "/home1.shtml"
#define LOG_CGI_RESPONE     "/alarm.shtml"
#define SNTP_CGI_RESPONE    "/sntp.shtml"
#define TRAP_CGI_RESPONE    "/snmp.shtml"
#define TFIX_CGI_RESPONE    "/timesw.shtml"
#define RECNT_CGI_RESPONSE  "/regate.shtml"
#define CTRSTYPE_CGI_RESPONSE "/config.shtml"
#define INFO_CGI_RESONSE      "/info.shtml"
#define UPD_CGI_RESONSE      "/udp.shtml"
#define RESPONSE_PAGE_SET_CGI_RSP_URL     "/response.ssi"
//*****************************************************************************
//
//! The file sent back to the browser in cases where a parameter error is
//! detected by one of the CGI handlers.  This should only happen if someone
//! tries to access the CGI directly via the browser command line and doesn't
//! enter all the required parameters alongside the URI.
//
//*****************************************************************************
#define PARAM_ERROR_RESPONSE    "/perror.shtml"

//*****************************************************************************
//
//! The file sent back to the browser to signal that the bootloader is being
//! entered to perform a software update.
//
//*****************************************************************************
#define FIRMWARE_UPDATE_RESPONSE "/blstart.shtml"

//*****************************************************************************
//
//! The file sent back to the browser to signal that the IP address of the
//! device is about to change and that the web server is no longer operating.
//
//*****************************************************************************
#define IP_UPDATE_RESPONSE  "/ipchg.shtml"

//*****************************************************************************
//
//! The URI of the ``Miscellaneous Settings'' page offered by the web server.
//
//*****************************************************************************
#define MISC_PAGE_URI           "/misc.shtml"

//*****************************************************************************
//
// Strings used for format JavaScript parameters for use by the configuration
// web pages.
//
//*****************************************************************************
#define JAVASCRIPT_HEADER                                                     \
    "<script type='text/javascript' language='JavaScript'><!--\n"
#define CTR_JAVASCRIPT_VARS                                                   \
    "var cm1 = %d;\n"                                                          \
    "var cm2 = %d;\n"                                                          \
    "var cm3 = %d;\n"                                                          \
    "var cm4 = %d;\n"

#define MOD_JAVASCRIPT_VARS                                                   \
    "var tn = %d;\n"                                                          \
    "var prf1 = %d;\n"                                                          \
    "var prf2 = %d;\n"                                                          \
    "var prf3 = %d;\n"                                                          \
    "var prf4 = %d;\n"                                                         \
        "var prf5 = %d;\n"                                                         \
        "var prf6 = %d;\n"                                                         \
        "var sm = %d;\n"                                                          \
         "var ss = %d;\n"


#define IP_JAVASCRIPT_VARS                                                    \
    "var staticip = %d;\n"                                                    \
    "var sip1 = %d;\n"                                                        \
    "var sip2 = %d;\n"                                                        \
    "var sip3 = %d;\n"                                                        \
    "var sip4 = %d;\n"
#define SUBNET_JAVASCRIPT_VARS                                                \
    "var syslog = %d;\n"                                                        \
    "var mip1 = %d;\n"                                                        \
    "var mip2 = %d;\n"                                                        \
    "var mip3 = %d;\n"                                                        \
    "var mip4 = %d;\n"
#define GW_JAVASCRIPT_VARS                                                    \
    "var gip1 = %d;\n"                                                        \
    "var gip2 = %d;\n"                                                        \
    "var gip3 = %d;\n"                                                        \
    "var gip4 = %d;\n"
#define NTP_JAVASCRIPT_VARS                                                \
    "var nip1 = %d;\n"                                                        \
    "var nip2 = %d;\n"                                                        \
    "var nip3 = %d;\n"                                                        \
    "var nip4 = %d;\n"                                                        \
        "var kpt = %d;\n"
#define DNS_JAVASCRIPT_VARS                                                \
    "var dns1 = %d;\n"                                                        \
    "var dns2 = %d;\n"                                                        \
    "var dns3 = %d;\n"                                                        \
    "var dns4 = %d;\n"                                                        \
        "var wpt = %d;\n"
#define SNTP_JAVASCRIPT_VARS                                                \
    "var sntpen = %d;\n"

#define SNMP_JAVASCRIPT_VARS                                                \
    "var trapen = %d;\n"
#define SWITCH_JAVASCRIPT_VARS                                               \
    "var swen1 = %d;\n"                                                          \
        "var swen2 = %d;\n"                                                          \
        "var swen3 = %d;\n"                                                          \
        "var swen4 = %d;\n"
#define BOOTH_JAVASCRIPT_VARS                                               \
    "var ch1on1 = %d;\n"                                                         \
        "var ch2on1 = %d;\n"                                                         \
        "var ch3on1 = %d;\n"                                                         \
        "var ch4on1 = %d;\n"
#define BOOTM_JAVASCRIPT_VARS                                               \
    "var ch1on2 = %d;\n"                                                         \
        "var ch2on2 = %d;\n"                                                         \
        "var ch3on2 = %d;\n"                                                         \
        "var ch4on2 = %d;\n"
#define SHUTH_JAVASCRIPT_VARS                                               \
    "var ch1on3 = %d;\n"                                                         \
        "var ch2on3 = %d;\n"                                                         \
        "var ch3on3 = %d;\n"                                                         \
        "var ch4on3 = %d;\n"
#define SHUTM_JAVASCRIPT_VARS                                               \
    "var ch1on4 = %d;\n"                                                         \
        "var ch2on4 = %d;\n"                                                         \
        "var ch3on4 = %d;\n"                                                         \
        "var ch4on4 = %d;\n"
#define REGATE_JAVASCRIPT_VARS                                                \
    "var rgcnt = %d;\n"
#define JAVASCRIPT_FOOTER                                                     \
    "//--></script>\n"

//*****************************************************************************
//
//! Structure used in mapping numeric IDs to human-readable strings.
//
//*****************************************************************************
typedef struct
{
    //
    //! A human readable string related to the identifier found in the ucId
    //! field.
    //
    const char* pcString;

    //
    //! An identifier value associated with the string held in the pcString
    //! field.
    //
    unsigned char ucId;
}
tStringMap;

//*****************************************************************************
//
//! The array used to map between parity identifiers and their human-readable
//! equivalents.
//
//*****************************************************************************
// static const tStringMap g_psParityMap[] =
// {
//    { "None", 0 },
//    { "Odd", 1 },
//    { "Even", SERIAL_PARITY_EVEN },
//    { "Mark", SERIAL_PARITY_MARK },
//    { "Space", SERIAL_PARITY_SPACE }
// };

#define NUM_PARITY_MAPS         (sizeof(g_psParityMap) / sizeof(tStringMap))
////*****************************************************************************
////
////! The array used to map between Time zone identifiers and their human-readable
////! equivalents.
////
////*****************************************************************************
//static const tStringMap g_psTimezoneMap[] =
//{
//   { "Paris UTC+01:00", 1 },
//   { "Athens UTC+02:00",  2 },
//   { "Baghdad UTC+03:00", 3 },
//   { "Abu Dhabi UTC+04:00", 4 },
//   { "Islamabad UTC+05:00",5 },
//   { "Dhaka UTC+06:00",  6 },
//   { "Bangkok UTC+07:00",7 },
//   { "Beijing UTC+08:00",8 },
//   { "Tokyo UTC+09:00", 9 },
//   { "Canberra  UTC+10:00",10 },
//   { "Vila UTC+11:00", 11 },
//   { "Wellington UTC+12:00",  12 },
//   { "London UTC+00:00", 13 },
//   { "The cape verde island UTC-01:00", 14 },
//   { "UTC-02:00",15 },
//   { "Buenos Aires UTC-03:00", 16 },
//   { "Santiago UTC-04:00",  17 },
//   { "Bogota UTC-05:00", 18 },
//   { "Moxico City UTC-06:00", 19 },
//   { "Arizona  UTC-07:00",20 },
//    { "Pacific time  UTC-08:00",21 },
//   { "Alaska  UTC-09:00",  22 },
//   { "Hawaii  UTC-10:00", 23 },
//   { "UTC-11:00", 24 },
//   { "The International Data Line West  UTC-12:00",25 }
//};

//#define NUM_TIMEZONE_MAPS         (sizeof(g_psTimezoneMap) / sizeof(tStringMap))

////*****************************************************************************
////
////! The array used to map between Switch mode identifiers and their human-readable
////! equivalents.
////
////*****************************************************************************
//static const tStringMap g_psSwitchmodeMap[] =
//{
//   { "Manual", 1 },
//   { "Auto",  2 }
//
//};

//#define NUM_SWITCHMODE_MAPS         (sizeof(g_psSwitchmodeMap) / sizeof(tStringMap))
//*****************************************************************************
//
//! The array used to map between Switch machine identifiers and their human-readable
//! equivalents.
//
//*****************************************************************************
//static const tStringMap g_psSwitchmachineMap[] =
//{
//   { "On", 1 },
//   { "Off",  2 }
//
//};
//
//#define NUM_SWITCHMACHINE_MAPS         (sizeof(g_psSwitchmachineMap) / sizeof(tStringMap))
//*****************************************************************************
//
//! The array used to map between flow control identifiers and their
//! human-readable equivalents.
//
//*****************************************************************************
// static const tStringMap g_psFlowControlMap[] =
// {
//    { "None", SERIAL_FLOW_CONTROL_NONE },
//    { "Hardware", SERIAL_FLOW_CONTROL_HW }
// };

//#define NUM_FLOW_CONTROL_MAPS   (sizeof(g_psFlowControlMap) / \
//                                 sizeof(tStringMap))

//*****************************************************************************
//
//! This structure instance contains the factory-default set of configuration
//! parameters for S2E module.
//
//*****************************************************************************
static const tConfigParameters g_sParametersFactory =
{
    //
    // The sequence number (ucSequenceNum); this value is not important for
    // the copy in SRAM.
    //
    0xAA,

    //
    // The CRC (ucCRC); this value is not important for the copy in SRAM.
    //
    0xA5,

    //
    // The parameter block version number (ucVersion).
    //
    0,

    //
    // Flags (ucFlags)
    //
    0x07,   //(//static ip 0x01  ,time sw en 0x02   TRAP EN 0x04)

    //
    // Flags (ucSyslog)
    //
    0x00,
    //
    // The TCP port number for UPnP discovery/response (usLocationURLPort).
    //
    6432,

    //
    // Reserved (ucReserved1).
    //
    {
        0, 0
    },


    //
    // Parameters for Port 0 (sPort[0]).
    //
    {
        //
        // The baud rate (ulBaudRate).
        //
        9600,

        //
        // The number of data bits.
        //
        8,

        //
        // The parity (NONE).
        //
        0,//SERIAL_PARITY_NONE,

        //
        // The number of stop bits.
        //
        1,

        //
        // The flow control (NONE).
        //
        0,//SERIAL_FLOW_CONTROL_NONE,

        //
        // The telnet session timeout (ulTelnetTimeout).
        //
        0,

        //
        // The telnet session listen or local port number
        // (usTelnetLocalPort).
        //
        3000,

        //
        // The telnet session remote port number (when in client mode).
        //
        3000,

        //
        // The IP address to which a connection will be made when in telnet
        // client mode.  This defaults to an invalid address since it's not
        // sensible to have a default value here.
        //
        0x00000000,

        //
        // Flags indicating the operating mode for this port.
        //
        PORT_RAW_CLIENT,

        //
        // Reserved (ucReserved0).
        //
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
    },



    //
    // Module Name (ucModName).
    //
    {
        'E', 'P', 'M', 'A', '1', '4', 'P', '-', '3', 0, 0, 0, 0, 0, 0
    },
    //
    // MAC ADDRESS
    //
    {
        0x00, 0x04, 0x76, 0x72, 0xb6, 0x62
    },
/* 2018-06-13 changzehai(DTT) ----------------------------------- MOD Start -*/
/* 修改IP地址和网关地址 */
//    //
//    // Static IP address (used only if indicated in ucFlags).
//    //
//    0xc0a80064,
//
//    //
//    // Default gateway IP address (used only if static IP is in use).
//    //
//    0xc0a80001,
    
    //
    // Static IP address (used only if indicated in ucFlags).
    //
    0xac120558, /* 172.18.5.88 */
    //
    // Default gateway IP address (used only if static IP is in use).
    //
    0xac1205fe, /* 172.18.5.254 */   
/* 2018-06-13 changzehai(DTT) ----------------------------------- MOD End   -*/    

    //
    // Subnet mask (used only if static IP is in use).
    //
    0xFFFFFF00,

    //
    //DNS ADDRESS
    //
    0xcf6110c3,
    //
    // USER PASSWORD
    //
    {
        'a', 'd', 'm', 'i', 'n',
    },
    //
    // boot time
    //
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    //
    // shut time
    //
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    //
    // SNTP Interval Seconds
    //
    60,
    //
    // SNTP trap service ip
    //
    0x85f3eef3,
    //
    // SNTP trap service URL
    //
    {
        "133.243.238.243"
    },
    //
    // community string
    //
    {
        'P', 'U', 'B', 'L', 'I', 'C', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    //
    // trap service ip address
    //
    {
        "172.18.5.89"
    },
    //
    // trap port
    //
    162,
    //
    // web port
    //
    80,
    //
    // Device Infomation
    //
    /*MODEL NAME*/
    {
        'E', 'P', 'M', 'A', '1', '4', 'P', '-', '3', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    /*Manufacturer NAME*/
    {
        "---"
    },
    /*serial*/
    {
        '2', '0', '1', '5', '1', '1', '0', '0', '3', '0', '0', '1', 0
    },
    /*Install Position*/
    {
        "上海浦东"
    },
    /*Install Persion*/
    {
        "---"
    },
    /*Install time*/
    {
        "2015-11-2 12:12:12"
    },
    /*run seconds*/
    0,
    //
    // Power Gate Parameters
    // enlarge 100 times
    /*over voltage*/
    24000,
    /*uvlo*/
    20000,
    /*voltage deadband*/
    500,
    /*over current*/
    1000,
    /*current deadband*/
    20,
    /*leak current*/
    1500,
    //
    // ch1---ch4 Parameters
    //
    /*remote cmd and channel time enable*/
    0x00,
    //
    //regate count and delay first \second\thirth
    //
    0x00,
    1,
    10,
    20,
    {'u', 'p', 'o', 'k'},
    {
        'a', 'd', 'm', 'i', 'n',
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    }
};
tSysCALParameters g_FactorySyscalparameters =
{
    0xAA,
    0xA5,
    0,
    0,
    100,
    100,
    0xFFFF,
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',}
};

tSysCALParameters g_sSyscalparaters;
//*****************************************************************************
//
//! This structure instance contains the run-time set of configuration
//! parameters for S2E module.  This is the active parameter set and may
//! contain changes that are not to be committed to flash.
//
//*****************************************************************************
tConfigParameters g_sParameters;

//*****************************************************************************
//
//! This structure instance points to the most recently saved parameter block
//! in flash.  It can be considered the default set of parameters.
//
//*****************************************************************************
const tConfigParameters* g_psDefaultParameters;

//*****************************************************************************
//
//! This structure contains the latest set of parameter committed to flash
//! and is used by the configuration pages to store changes that are to be
//! written back to flash.  Note that g_sParameters may contain other changes
//! which are not to be written so we can't merely save the contents of the
//! active parameter block if the user requests some change to the defaults.
//
//*****************************************************************************
tConfigParameters g_sWorkingDefaultParameters;

//*****************************************************************************
//
//! This structure instance points to the factory default set of parameters in
//! flash memory.
//
//*****************************************************************************
const tConfigParameters* const g_psFactoryParameters = &g_sParametersFactory;

//*****************************************************************************
//
//! The version of the firmware.  Changing this value will make it much more
//! difficult for Luminary Micro support personnel to determine the firmware in
//! use when trying to provide assistance; it should only be changed after
//! careful consideration.
//
//*****************************************************************************
const unsigned short g_usFirmwareVersion = 3742;
tBoolean syslog = false;
//*****************************************************************************
//
//! Loads the S2E parameter block from factory-default table.
//!
//! This function is called to load the factory default parameter block.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigLoadDeafault( void )
{
    //
    // Copy the factory default parameter set to the active and working
    // parameter blocks.g_sWorkingDefaultParameters
    //
    //g_sParameters = g_sParametersFactory;
    g_sWorkingDefaultParameters = g_sParametersFactory;
    g_sWorkingDefaultParameters.ulStaticIP = g_sParameters.ulStaticIP;
    g_sWorkingDefaultParameters.ulGatewayIP = g_sParameters.ulGatewayIP;
    g_sWorkingDefaultParameters.ulSubnetMask = g_sParameters.ulSubnetMask;
    memcpy( &g_sWorkingDefaultParameters.ulMACAddr[0], &g_sParameters.ulMACAddr[0], 6 ); //MAC Address
    memcpy( g_sWorkingDefaultParameters.Serial, g_sParametersFactory.Serial, 13 );
}

//*****************************************************************************
//
//! Loads the S2E parameter block from factory-default table.
//!
//! This function is called to load the factory default parameter block.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigLoadFactory( void )
{
    //
    // Copy the factory default parameter set to the active and working
    // parameter blocks.
    //
    g_sParameters = g_sParametersFactory;
    g_sWorkingDefaultParameters = g_sParametersFactory;
}
//*****************************************************************************
//
//! Gets the address of the most recent parameter block.
//!
//! This function returns the address of the most recent parameter block that
//! is stored in flash.
//!
//! \return Returns the address of the most recent parameter block, or NULL if
//! there are no valid parameter blocks in flash.
//
//*****************************************************************************
unsigned char*
FlashPBGet( void )
{
    //
    // See if there is a valid parameter block.
    //
    if( g_pucFlashPBCurrent )
    {
        //
        // Return the address of the most recent parameter block.
        //
        return( g_pucFlashPBCurrent );
    }
    //
    // There are no valid parameter blocks in flash, so return NULL.
    //
    return( 0 );
}
//*****************************************************************************
//
//! Loads the S2E parameter block from flash.
//!
//! This function is called to load the most recently saved parameter block
//! from flash.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigLoad( void )
{
    unsigned char* pucBuffer = NULL;
    //
    // Get a pointer to the latest parameter block in flash.
    //
    pucBuffer = FlashPBGet();
    //
    // See if a parameter block was found in flash.
    //
    if( pucBuffer )
    {
        //
        // A parameter block was found so copy the contents to both our
        // active parameter set and the working default set.
        //
        g_sParameters = *( tConfigParameters* )pucBuffer;
        g_sWorkingDefaultParameters = g_sParameters;
    }
}
void
FlashPBSave( unsigned char* pucBuffer )
{
    //unsigned char *pucNew;
    //unsigned long ulIdx, ulSum;
    //static FLASH_EraseInitTypeDef EraseInitStruct;
    //uint32_t FirstSector = 0, NbOfSectors = 0;
    //uint32_t SectorError = 0;
    //
    // Check the arguments.
    //
    assert_param( pucBuffer != ( void* )0 );
    pucBuffer[0] = 0xA5;
    pucBuffer[1] = 0xAA;
    //
    // erase the block of 4096 size
    //
    SPI_FLASH_SectorErase( SSICONFIG_ADDR );
    SPI_FLASH_BufferWrite( pucBuffer, SSICONFIG_ADDR, sizeof( tConfigParameters ) );
}
void FlashSYSSave( unsigned char* pucBuffer )
{
    assert_param( pucBuffer != ( void* )0 );
    pucBuffer[0] = 0xAA;
    pucBuffer[1] = 0xA5;
    //
    // erase the block of 4096 size
    //
    SPI_FLASH_SectorErase( SSICONFIG_ADDR + CACHE_BLOCK_SIZE );
    SPI_FLASH_BufferWrite( pucBuffer, SSICONFIG_ADDR + CACHE_BLOCK_SIZE, sizeof( tSysCALParameters ) );
}
//*****************************************************************************
//
//! Saves the S2E parameter block to flash.
//!
//! This function is called to save the current S2E configuration parameter
//! block to flash memory.
//!
//! \return None.
//
//*****************************************************************************
void SystemSave( void )
{
    FlashSYSSave( ( unsigned char* )&g_sSyscalparaters );
}
void
ConfigSave( void )
{
    unsigned char* pucBuffer;
    //
    // Save the working defaults parameter block to flash.
    //
    FlashPBSave( ( unsigned char* )&g_sWorkingDefaultParameters );
    //
    // Get the pointer to the most recenly saved buffer.
    // (should be the one we just saved).
    //
    //pucBuffer = FlashPBGet();
    //
    // Update the default parameter pointer.
    //
    if( pucBuffer )
    {
        g_psDefaultParameters = ( tConfigParameters* )pucBuffer;
    }
    else
    {
        g_psDefaultParameters = ( tConfigParameters* )g_psFactoryParameters;
    }
}

//*****************************************************************************
//
//! Initializes the configuration parameter block.
//!
//! This function initializes the configuration parameter block.  If the
//! version number of the parameter block stored in flash is older than
//! the current revision, new parameters will be set to default values as
//! needed.
//!
//! \return None.
//
//*****************************************************************************
/* Private functions ---------------------------------------------------------*/
static unsigned char CurrentConfig[FLASH_CONFIG_SIZE];
static unsigned char CurrentSys[FLASH_SYCCAL_SIZE ];
void ConfigInit( void )
{
    unsigned char* pucBuffer;
    struct ip_addr pucremoteIP;
    char* pucurl;
    u32_t pucIPaddr;
    //
    // Verify that the parameter block structure matches the FLASH parameter
    // block size.
    //
    //
    // Read back config parameters
    //
    SPI_FLASH_BufferRead( CurrentConfig, SSICONFIG_ADDR, sizeof( tConfigParameters ) );
    SPI_FLASH_BufferRead( CurrentSys, SSICONFIG_ADDR + CACHE_BLOCK_SIZE, sizeof( tSysCALParameters ) );
    //
    //check the config flag.
    //
    if( ( CurrentConfig[0] == 0xA5 ) && ( CurrentConfig[1] == 0xAA ) )
    {
        g_pucFlashPBCurrent = CurrentConfig;
    }
    else
    {
        g_pucFlashPBCurrent = 0;
    }
    if( ( CurrentSys[0] == 0xAA ) && ( CurrentSys[1] == 0xA5 ) )
    {
        g_sSyscalparaters = *( tSysCALParameters* )CurrentSys;
    }
    else
    {
        g_sSyscalparaters = g_FactorySyscalparameters;
    }
    //
    // First, load the factory default values.g_pucFlashSYSCurrent
    //
    ConfigLoadFactory();
    //
    // Then, if available, load the latest non-volatile set of values.
    //
    ConfigLoad();
    //
    // Get the pointer to the most recently saved buffer.
    //
    pucBuffer = FlashPBGet();
    //
    // Update the default parameter pointer.
    //
    if( pucBuffer )
    {
        g_psDefaultParameters = ( tConfigParameters* )pucBuffer;
    }
    else
    {
        g_psDefaultParameters = ( tConfigParameters* )g_psFactoryParameters;
        //ConfigSave();
    }
    //
    //!snmp trap var iniliztion.
    //
    //    err = ipaddr_aton(sntp_server_addresses[sntp_current_server], &sntp_server_address)? ERR_OK : ERR_ARG;
    //
    //! get host IPAdress saved point.
    //
    pucurl = ( char* )g_sParameters.TrapService;
    //
    //! get the ip addr from the url.
    //
    pucremoteIP.addr = inet_addr( ( char* )pucurl );
    //
    //! if the ipaddr get from the url nozero.
    if( pucremoteIP.addr != INADDR_NONE )
    {
        //
        // ! ip addr transmite to the long.
        //
        pucIPaddr = htonl( pucremoteIP.addr );
        //
        // get the host addr from the pucIPaddr.
        //
        HostAddr[0] = ( pucIPaddr >> 24 ) & 0xff;
        HostAddr[1] = ( pucIPaddr >> 16 ) & 0xff;
        HostAddr[2] = ( pucIPaddr >> 8 ) & 0xff;
        HostAddr[3] =  pucIPaddr & 0xff ;
    }
    memcpy( MACAddress, &g_sParameters.ulMACAddr[0], 6 ); //MAC Address
}

//*****************************************************************************
//
//! Configures HTTPD server SSI and CGI capabilities for our configuration
//! forms.
//!
//! This function informs the HTTPD server of the server-side-include tags
//! that we will be processing and the special URLs that are used for
//! CGI processing for the web-based configuration forms.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigWebInit( void )
{
    //
    // Pass our tag information to the HTTP server.
    //
    http_set_ssi_handler( ConfigSSIHandler, g_pcConfigSSITags,
                          NUM_CONFIG_SSI_TAGS );
    //
    // Pass our CGI handlers to the HTTP server.
    //
    http_set_cgi_handlers( g_psConfigCGIURIs, NUM_CONFIG_CGI_URIS );
}

//*****************************************************************************
//
//! \internal
//!
//! Searches a mapping array to find a human-readable description for a
//! given identifier.
//!
//! \param psMap points to an array of \e tStringMap structures which contain
//! the mappings to be searched for the provided identifier.
//! \param ulEntries contains the number of map entries in the \e psMap array.
//! \param ucId is the identifier whose description is to be returned.
//!
//! This function scans the given array of ID/string maps and returns a pointer
//! to the string associated with the /e ucId parameter passed.  If the
//! identifier is not found in the map array, a pointer to ``**UNKNOWN**'' is
//! returned.
//!
//! \return Returns a pointer to an ASCII string describing the identifier
//! passed, if found, or ``**UNKNOWN**'' if not found.
//
//*****************************************************************************
static const char*
ConfigMapIdToString( const tStringMap* psMap, unsigned long ulEntries,
                     unsigned char ucId )
{
    unsigned long ulLoop;
    //
    // Check each entry in the map array looking for the ID number we were
    // passed.
    //
    for( ulLoop = 0; ulLoop < ulEntries; ulLoop++ )
    {
        //
        // Does this map entry match?
        //
        if( psMap[ulLoop].ucId == ucId )
        {
            //
            // Yes - return the IDs description string.
            //
            return( psMap[ulLoop].pcString );
        }
    }
    //
    // If we drop out, the ID passed was not found in the map array so return
    // a default "**UNKNOWN**" string.
    //
    return( "**UNKNOWN**" );
}

//*****************************************************************************
//
//! \internal
//!
//! Updates all parameters associated with a single port.
//!
//! \param ulPort specifies which of the two supported ports to update.  Valid
//! values are 0 and 1.
//!
//! This function changes the serial and telnet configuration to match the
//! values stored in g_sParameters.sPort for the supplied port.  On exit, the
//! new parameters will be in effect and g_sParameters.sPort will have been
//! updated to show the actual parameters in effect (in case any of the
//! supplied parameters are not valid or the actual hardware values differ
//! slightly from the requested value).
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdatePortParameters( unsigned long ulPort, tBoolean bSerial,
                            tBoolean bTelnet )
{
    //
    // Do we have to update the telnet settings?  Note that we need to do this
    // first since the act of initiating a telnet connection resets the serial
    // port settings to defaults.
    //
    //     if(bTelnet)
    //     {
    //         //
    //         // Close any existing connection and shut down the server if required.
    //         //
    //         TelnetClose(ulPort);
    //         //
    //         // Are we to operate as a telnet server?
    //         //
    //         if((g_sParameters.sPort[ulPort].ucFlags & PORT_FLAG_TELNET_MODE) ==
    //            PORT_TELNET_SERVER)
    //         {
    //             //
    //             // Yes - start listening on the required port.
    //             //
    //             TelnetListen(g_sParameters.sPort[ulPort].usTelnetLocalPort,
    //                          ulPort);
    //         }
    //         else
    //         {
    //             //
    //             // No - we are a client so initiate a connection to the desired
    //             // IP address using the configured ports.
    //             //
    //             TelnetOpen(g_sParameters.sPort[ulPort].ulTelnetIPAddr,
    //                        g_sParameters.sPort[ulPort].usTelnetRemotePort,
    //                        g_sParameters.sPort[ulPort].usTelnetLocalPort,
    //                        ulPort);
    //         }
    //     }
    //     //
    //     // Do we need to update the serial port settings?  We do this if we are
    //     // told that the serial settings changed or if we just reconfigured the
    //     // telnet settings (which resets the serial port parameters to defaults as
    //     // a side effect).
    //     //
    //     if(bSerial || bTelnet)
    //     {
    //         SerialSetCurrent(ulPort);
    //     }
}

//*****************************************************************************
//
//! \internal
//!
//! Performs any actions necessary in preparation for a change if IP address.
//!
//! This function is called before ConfigUpdateIPAddress to remove the device
//! from the UPnP network in preparation for a change of IP address or
//! switch between DHCP and StaticIP.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigPreUpdateIPAddress( void )
{
    //
    // Stop UPnP and remove ourselves from the network.
    //
    //UPnPStop();
    sntp_stop();
}

//*****************************************************************************
//
//! \internal
//!
//! Sets the IP address selection mode and associated parameters.
//!
//! This function ensures that the IP address selection mode (static IP or
//! DHCP/AutoIP) is set according to the parameters stored in g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdateIPAddress( void )
{
    sntp_stop();
    //
    // Change to static/dynamic based on the current settings in the
    // global parameter block.
    //
    if( ( g_sParameters.ucFlags & CONFIG_FLAG_STATICIP ) == CONFIG_FLAG_STATICIP )
    {
        lwIPNetworkConfigChange( g_sParameters.ulStaticIP,
                                 g_sParameters.ulSubnetMask,
                                 g_sParameters.ulGatewayIP,
                                 IPADDR_USE_STATIC );
    }
    else
    {
        lwIPNetworkConfigChange( 0, 0, 0, IPADDR_USE_DHCP );
    }
    //     //
    //     // Restart UPnP discovery.
    //     //
    //     UPnPStart();
    //
    // Restart sntp handle.
    //
    sntp_init();
}

//*****************************************************************************
//
//! \internal
//!
//! Performs changes as required to apply all active parameters to the system.
//!
//! This function ensures that the system configuration matches the values in
//! the current, active parameter block.  It is called after the parameter
//! block has been reset to factory defaults.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdateAllParameters( void )
{
    //
    // Update the IP address selection parameters.
    //
    ConfigPreUpdateIPAddress();
    ConfigUpdateIPAddress();
    //     //
    //     // Update the parameters for each of the individual ports.
    //     //
    //     ConfigUpdatePortParameters(0, true, true);
    //     ConfigUpdatePortParameters(1, true, true);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler and returns the
//! index of a given parameter within that list.
//!
//! \param pcToFind is a pointer to a string containing the name of the
//! parameter that is to be found.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//!
//! This function searches an array of parameters to find the string passed in
//! \e pcToFind.  If the string is found, the index of that string within the
//! \e pcParam array is returned, otherwise -1 is returned.
//!
//! \return Returns the index of string \e pcToFind within array \e pcParam
//! or -1 if the string does not exist in the array.
//
//*****************************************************************************
static int
ConfigFindCGIParameter( const char* pcToFind, char* pcParam[], int iNumParams )
{
    int iLoop;
    //
    // Scan through all the parameters in the array.
    //
    for( iLoop = 0; iLoop < iNumParams; iLoop++ )
    {
        //
        // Does the parameter name match the provided string?
        //
        if( strcmp( pcToFind, pcParam[iLoop] ) == 0 )
        {
            //
            // We found a match - return the index.
            //
            return( iLoop );
        }
    }
    //
    // If we drop out, the parameter was not found.
    //
    return( -1 );
}

static tBoolean
ConfigIsValidHexDigit( const char cDigit )
{
    if( ( ( cDigit >= '0' ) && ( cDigit <= '9' ) ) ||
            ( ( cDigit >= 'a' ) && ( cDigit <= 'f' ) ) ||
            ( ( cDigit >= 'A' ) && ( cDigit <= 'F' ) ) )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

static unsigned char
ConfigHexDigit( const char cDigit )
{
    if( ( cDigit >= '0' ) && ( cDigit <= '9' ) )
    {
        return( cDigit - '0' );
    }
    else
    {
        if( ( cDigit >= 'a' ) && ( cDigit <= 'f' ) )
        {
            return( ( cDigit - 'a' ) + 10 );
        }
        else
        {
            if( ( cDigit >= 'A' ) && ( cDigit <= 'F' ) )
            {
                return( ( cDigit - 'A' ) + 10 );
            }
        }
    }
    //
    // If we get here, we were passed an invalid hex digit so return 0xFF.
    //
    return( 0xFF );
}

//*****************************************************************************
//
//! \internal
//!
//! Decodes a single %xx escape sequence as an ASCII character.
//!
//! \param pcEncoded points to the ``%'' character at the start of a three
//! character escape sequence which represents a single ASCII character.
//! \param pcDecoded points to a byte which will be written with the decoded
//! character assuming the escape sequence is valid.
//!
//! This function decodes a single escape sequence of the form ``%xy'' where
//! x and y represent hexadecimal digits.  If each digit is a valid hex digit,
//! the function writes the decoded character to the pcDecoded buffer and
//! returns true, else it returns false.
//!
//! \return Returns \b true on success or \b false if pcEncoded does not point
//! to a valid escape sequence.
//
//*****************************************************************************
static tBoolean
ConfigDecodeHexEscape( const char* pcEncoded, char* pcDecoded )
{
    if( ( pcEncoded[0] != '%' ) || !ConfigIsValidHexDigit( pcEncoded[1] ) ||
            !ConfigIsValidHexDigit( pcEncoded[2] ) )
    {
        return( false );
    }
    else
    {
        *pcDecoded = ( ( ConfigHexDigit( pcEncoded[1] ) * 16 ) +
                       ConfigHexDigit( pcEncoded[2] ) );
        return( true );
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Encodes a string for use within an HTML tag, escaping non alphanumeric
//! characters.
//!
//! \param pcDecoded is a pointer to a null terminated ASCII string.
//! \param pcEncoded is a pointer to a storage for the encoded string.
//! \param ulLen is the number of bytes of storage pointed to by pcEncoded.
//!
//! This function encodes a string, adding escapes in place of any special,
//! non-alphanumeric characters.  If the encoded string is too long for the
//! provided output buffer, the output will be truncated.
//!
//! \return Returns the number of characters written to the output buffer
//! not including the terminating NULL.
//
//*****************************************************************************
static unsigned long
ConfigEncodeFormString( const char* pcDecoded, char* pcEncoded,
                        unsigned long ulLen )
{
    unsigned long ulLoop;
    unsigned long ulCount;
    //
    // Make sure we were not passed a tiny buffer.
    //
    if( ulLen <= 1 )
    {
        return( 0 );
    }
    //
    // Initialize our output character counter.
    //
    ulCount = 0;
    //
    // Step through each character of the input until we run out of data or
    // space to put our output in.
    //
    for( ulLoop = 0; pcDecoded[ulLoop] && ( ulCount < ( ulLen - 1 ) ); ulLoop++ )
    {
        switch( pcDecoded[ulLoop] )
        {
            //
            // Pass most characters without modification.
            //
            default:
            {
                pcEncoded[ulCount++] = pcDecoded[ulLoop];
                break;
            }
            case '\'':
            {
                ulCount += usnprintf( &pcEncoded[ulCount], ( ulLen - ulCount ),
                                      "&#39;" );
                break;
            }
        }
    }
    return( ulCount );
}

//*****************************************************************************
//
//! \internal
//!
//! Decodes a string encoded as part of an HTTP URI.
//!
//! \param pcEncoded is a pointer to a null terminated string encoded as per
//! RFC1738, section 2.2.
//! \param pcDecoded is a pointer to storage for the decoded, null terminated
//! string.
//! \param ulLen is the number of bytes of storage pointed to by pcDecoded.
//!
//! This function decodes a string which has been encoded using the method
//! described in RFC1738, section 2.2 for URLs.  If the decoded string is too
//! long for the provided output buffer, the output will be truncated.
//!
//! \return Returns the number of character written to the output buffer, not
//! including the terminating NULL.
//
//*****************************************************************************
static unsigned long
ConfigDecodeFormString( const  char* pcEncoded, char* pcDecoded,
                        unsigned long ulLen )
{
    unsigned long ulLoop;
    unsigned long ulCount;
    tBoolean bValid;
    ulCount = 0;
    ulLoop = 0;
    //
    // Keep going until we run out of input or fill the output buffer.
    //
    while( pcEncoded[ulLoop] && ( ulCount < ( ulLen - 1 ) ) )
    {
        switch( pcEncoded[ulLoop] )
        {
            //
            // '+' in the encoded data is decoded as a space.
            //
            case '+':
            {
                pcDecoded[ulCount++] = ' ';
                ulLoop++;
                break;
            }
            //
            // '%' in the encoded data indicates that the following 2
            // characters indicate the hex ASCII code of the decoded character.
            //
            case '%':
            {
                if( pcEncoded[ulLoop + 1] && pcEncoded[ulLoop + 2] )
                {
                    //
                    // Decode the escape sequence.
                    //
                    bValid = ConfigDecodeHexEscape( &pcEncoded[ulLoop],
                                                    &pcDecoded[ulCount] );
                    //
                    // If the escape sequence was valid, move to the next
                    // output character.
                    //
                    if( bValid )
                    {
                        ulCount++;
                    }
                    //
                    // Skip past the escape sequence in the encoded string.
                    //
                    ulLoop += 3;
                }
                else
                {
                    //
                    // We reached the end of the string partway through an
                    // escape sequence so just ignore it and return the number
                    // of decoded characters found so far.
                    //
                    pcDecoded[ulCount] = '\0';
                    return( ulCount );
                }
                break;
            }
            //
            // For all other characters, just copy the input to the output.
            //
            default:
            {
                pcDecoded[ulCount++] = pcEncoded[ulLoop++];
                break;
            }
        }
    }
    //
    // Terminate the string and return the number of characters we decoded.
    //
    pcDecoded[ulCount] = '\0';
    return( ulCount );
}

//*****************************************************************************
//
//! \internal
//!
//! Ensures that a string passed represents a valid decimal number and,
//! if so, converts that number to a long.
//!
//! \param pcValue points to a null terminated string which should contain an
//! ASCII representation of a decimal number.
//! \param plValue points to storage which will receive the number represented
//! by pcValue assuming the string is a valid decimal number.
//!
//! This function determines whether or not a given string represents a valid
//! decimal number and, if it does, converts the string into a decimal number
//! which is returned to the caller.
//!
//! \return Returns \b true if the string is a valid representation of a
//! decimal number or \b false if not.

//*****************************************************************************
static tBoolean
ConfigCheckDecimalParam( const char* pcValue, long* plValue )
{
    unsigned long ulLoop;
    tBoolean bStarted;
    tBoolean bFinished;
    tBoolean bNeg;
    long lAccum;
    //
    // Check that the string is a valid decimal number.
    //
    bStarted = false;
    bFinished = false;
    bNeg = false;
    ulLoop = 0;
    lAccum = 0;
    while( pcValue[ulLoop] )
    {
        //
        // Ignore whitespace before the string.
        //
        if( !bStarted )
        {
            if( ( pcValue[ulLoop] == ' ' ) || ( pcValue[ulLoop] == '\t' ) )
            {
                ulLoop++;
                continue;
            }
            //
            // Ignore a + or - character as long as we have not started.
            //
            if( ( pcValue[ulLoop] == '+' ) || ( pcValue[ulLoop] == '-' ) )
            {
                //
                // If the string starts with a '-', remember to negate the
                // result.
                //
                bNeg = ( pcValue[ulLoop] == '-' ) ? true : false;
                bStarted = true;
                ulLoop++;
            }
            else
            {
                //
                // We found something other than whitespace or a sign character
                // so we start looking for numerals now.
                //
                bStarted = true;
            }
        }
        if( bStarted )
        {
            if( !bFinished )
            {
                //
                // We expect to see nothing other than valid digit characters
                // here.
                //
                if( pcValue[ulLoop] == '.' )
                {
                    ulLoop++;
                }
                if( ( pcValue[ulLoop] >= '0' ) && ( pcValue[ulLoop] <= '9' ) )
                {
                    lAccum = ( lAccum * 10 ) + ( pcValue[ulLoop] - '0' );
                }
                else
                {
                    //
                    // Have we hit whitespace?  If so, check for no more
                    // characters until the end of the string.
                    //
                    if( ( pcValue[ulLoop] == ' ' ) || ( pcValue[ulLoop] == '\t' ) )
                    {
                        bFinished = true;
                    }
                    else
                    {
                        //
                        // We got something other than a digit or whitespace so
                        // this makes the string invalid as a decimal number.
                        //
                        return( false );
                    }
                }
            }
            else
            {
                //
                // We are scanning for whitespace until the end of the string.
                //
                if( ( pcValue[ulLoop] != ' ' ) && ( pcValue[ulLoop] != '\t' ) )
                {
                    //
                    // We found something other than whitespace so the string
                    // is not valid.
                    //
                    return( false );
                }
            }
            //
            // Move on to the next character in the string.
            //
            ulLoop++;
        }
    }
    //
    // If we drop out of the loop, the string must be valid.  All we need to do
    // now is negate the accumulated value if the string started with a '-'.
    //
    *plValue = bNeg ? -lAccum : lAccum;
    return( true );
}
//*****************************************************************************
//
//! \internal  240.00  ->24000   240.0 ->24000  240 ->24000
//!
//! Ensures that a string passed represents a valid decimal number and,
//! if so, converts that number to a long.
//!
//! \param pcValue points to a null terminated string which should contain an
//! ASCII representation of a decimal number.
//! \param plValue points to storage which will receive the number represented
//! by pcValue assuming the string is a valid decimal number.
//!
//! This function determines whether or not a given string represents a valid
//! decimal number and, if it does, converts the string into a decimal number
//! which is returned to the caller.
//!
//! \return Returns \b true if the string is a valid representation of a
//! decimal number or \b false if not.

//*****************************************************************************
static tBoolean
ConfigCheckDecimalParam1( const char* pcValue, long* plValue )
{
    unsigned long ulLoop;
    tBoolean bStarted;
    tBoolean bFinished;
    tBoolean bNeg;
    tBoolean bDecimal;
    unsigned char cAftDecimalPos = 0;
    long lAccum;
    //
    // Check that the string is a valid decimal number.
    //
    bStarted = false;
    bFinished = false;
    bNeg = false;
    bDecimal = false;
    ulLoop = 0;
    lAccum = 0;
    while( pcValue[ulLoop] )
    {
        //
        // Ignore whitespace before the string.
        //
        if( !bStarted )
        {
            if( ( pcValue[ulLoop] == ' ' ) || ( pcValue[ulLoop] == '\t' ) )
            {
                ulLoop++;
                continue;
            }
            //
            // Ignore a + or - character as long as we have not started.
            //
            if( ( pcValue[ulLoop] == '+' ) || ( pcValue[ulLoop] == '-' ) )
            {
                //
                // If the string starts with a '-', remember to negate the
                // result.
                //
                bNeg = ( pcValue[ulLoop] == '-' ) ? true : false;
                bStarted = true;
                ulLoop++;
            }
            else
            {
                //
                // We found something other than whitespace or a sign character
                // so we start looking for numerals now.
                //
                bStarted = true;
            }
        }
        if( bStarted )
        {
            if( !bFinished )
            {
                //
                // We expect to see nothing other than valid digit characters
                // here.
                //
                if( pcValue[ulLoop] == '.' )
                {
                    ulLoop++;
                    bDecimal = true;
                }
                if( ( pcValue[ulLoop] >= '0' ) && ( pcValue[ulLoop] <= '9' ) )
                {
                    lAccum = ( lAccum * 10 ) + ( pcValue[ulLoop] - '0' );
                    if( bDecimal )
                    {
                        cAftDecimalPos += 1;
                    }
                    if( cAftDecimalPos > 2 )
                    {
                        return( false );
                    }
                }
                else
                {
                    //
                    // Have we hit whitespace?  If so, check for no more
                    // characters until the end of the string.
                    //
                    if( ( pcValue[ulLoop] == ' ' ) || ( pcValue[ulLoop] == '\t' ) )
                    {
                        bFinished = true;
                    }
                    else
                    {
                        //
                        // We got something other than a digit or whitespace so
                        // this makes the string invalid as a decimal number.
                        //
                        return( false );
                    }
                }
            }
            else
            {
                //
                // We are scanning for whitespace until the end of the string.
                //
                if( ( pcValue[ulLoop] != ' ' ) && ( pcValue[ulLoop] != '\t' ) )
                {
                    //
                    // We found something other than whitespace so the string
                    // is not valid.
                    //
                    return( false );
                }
            }
            //
            // Move on to the next character in the string.
            //
            ulLoop++;
        }
    }
    //
    // If we drop out of the loop, the string must be valid.  All we need to do
    // now is negate the accumulated value if the string started with a '-'.
    //
    if( cAftDecimalPos == 0 )
    {
        lAccum *= 100;
    }
    else if( cAftDecimalPos == 1 )
    {
        lAccum *= 10;
    }
    *plValue = bNeg ? -lAccum : lAccum;
    return( true );
}
//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler for a parameter
//! with the given name and, if found, reads the parameter value as a decimal
//! number.
//!
//! \param pcName is a pointer to a string containing the name of the
//! parameter that is to be found.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//! \param pcValues is an array of values associated with each parameter from
//! the pcParam array.
//! \param pbError is a pointer that will be written to \b true if there is any
//! error during the parameter parsing process (parameter not found, value is
//! not a valid decimal number).
//!
//! This function searches an array of parameters to find the string passed in
//! \e pcName.  If the string is found, the corresponding parameter value is
//! read from array pcValues and checked to make sure that it is a valid
//! decimal number.  If so, the number is returned.  If any error is detected,
//! parameter \e pbError is written to \b true.  Note that \e pbError is NOT
//! written if the parameter is successfully found and validated.  This is to
//! allow multiple parameters to be parsed without the caller needing to check
//! return codes after each individual call.
//!
//! \return Returns the value of the parameter or 0 if an error is detected (in
//! which case \e *pbError will be \b true).
//
//*****************************************************************************
static long
ConfigGetCGIParam( const char* pcName, char* pcParams[], char* pcValue[],
                   int iNumParams, tBoolean* pbError )
{
    int iParam;
    long lValue;
    tBoolean bRetcode;
    //
    // Is the parameter we are looking for in the list?
    //
    lValue = 0;
    iParam = ConfigFindCGIParameter( pcName, pcParams, iNumParams );
    if( iParam != -1 )
    {
        //
        // We found the parameter so now get its value.
        //
        bRetcode = ConfigCheckDecimalParam( pcValue[iParam], &lValue );
        if( bRetcode )
        {
            //
            // All is well - return the parameter value.
            //
            return( lValue );
        }
    }
    //
    // If we reach here, there was a problem so return 0 and set the error
    // flag.
    //
    *pbError = true;
    return( 0 );
}
//240 ->24000 240.0 ->24000 240.00 ->24000
//*****************************************************************************
static long
ConfigGetCGIParam1( const char* pcName, char* pcParams[], char* pcValue[],
                    int iNumParams, tBoolean* pbError )
{
    int iParam;
    long lValue;
    tBoolean bRetcode;
    //
    // Is the parameter we are looking for in the list?
    //
    lValue = 0;
    iParam = ConfigFindCGIParameter( pcName, pcParams, iNumParams );
    if( iParam != -1 )
    {
        //
        // We found the parameter so now get its value.
        //
        bRetcode = ConfigCheckDecimalParam1( pcValue[iParam], &lValue );
        if( bRetcode )
        {
            //
            // All is well - return the parameter value.
            //
            return( lValue );
        }
    }
    //
    // If we reach here, there was a problem so return 0 and set the error
    // flag.
    //
    *pbError = true;
    return( 0 );
}
//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler for 4 parameters
//! representing an IP address and extracts the IP address defined by them.
//!
//! \param pcName is a pointer to a string containing the base name of the IP
//! address parameters.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//! \param pcValues is an array of values associated with each parameter from
//! the pcParam array.
//! \param pbError is a pointer that will be written to \b true if there is any
//! error during the parameter parsing process (parameter not found, value is
//! not a valid decimal number).
//!
//! This function searches an array of parameters to find four parameters
//! whose names are \e pcName appended with digits 1 - 4.  Each of these
//! parameters is expected to have a value which is a decimal number between
//! 0 and 255.  The parameter values are read and concatenated into an unsigned
//! long representing an IP address with parameter 1 in the leftmost postion.
//!
//! For example, if \e pcName points to string ``ip'', the function will look
//! for 4 CGI parameters named ``ip1'', ``ip2'', ``ip3'' and ``ip4'' and read
//! their values to generate an IP address of the form 0xAABBCCDD where ``AA''
//! is the value of parameter ``ip1'', ``BB'' is the value of ``p2'', ``CC''
//! is the value of ``ip3'' and ``DD'' is the value of ``ip4''.
//!
//! \return Returns the IP address read or 0 if an error is detected (in
//! which case \e *pbError will be \b true).
//
//*****************************************************************************
unsigned long
ConfigGetCGIIPAddr( const char* pcName, char* pcParam[], char* pcValue[],
                    int iNumParams, tBoolean* pbError )
{
    unsigned long ulIPAddr;
    unsigned long ulLoop;
    long lValue;
    char pcVariable[MAX_VARIABLE_NAME_LEN];
    tBoolean bError;
    //
    // Set up for the loop which reads each address element.
    //
    ulIPAddr = 0;
    bError = false;
    //
    // Look for each of the four variables in turn.
    //
    for( ulLoop = 1; ulLoop <= 4; ulLoop++ )
    {
        //
        // Generate the name of the variable we are looking for next.
        //
        usnprintf( pcVariable, MAX_VARIABLE_NAME_LEN, "%s%d", pcName, ulLoop );
        //
        // Shift our existing IP address to the left prior to reading the next
        // byte.
        //
        ulIPAddr <<= 8;
        //
        // Get the next variable and mask it into the IP address.
        //
        lValue = ConfigGetCGIParam( pcVariable, pcParam, pcValue, iNumParams,
                                    &bError );
        ulIPAddr |= ( ( unsigned long )lValue & 0xFF );
    }
    //
    // Did we encounter any error while reading the parameters?
    //
    if( bError )
    {
        //
        // Yes - mark the clients error flag and return 0.
        //
        *pbError = true;
        return( 0 );
    }
    else
    {
        //
        // No - all is well so return the IP address.
        //
        return( ulIPAddr );
    }
}
//String into an integer
unsigned char StringtoInt( char* s )
{
    unsigned lvaule = 0;
    if( ( s[0] >= '0' ) && ( s[0] <= '9' ) )
    {
        lvaule = s[0] - '0';
    }
    else
    {
        if( ( s[0] >= 'A' ) && ( s[0] <= 'F' ) )
        {
            lvaule = s[0] - 'A' + 10;
        }
        else if( ( s[0] >= 'a' ) && ( s[0] <= 'f' ) )
        {
            lvaule = s[0] - 'a' + 10;
        }
    }
    if( ( s[1] >= '0' ) && ( s[1] <= '9' ) )
    {
        lvaule  = ( lvaule << 4 ) | ( s[1] - '0' );
    }
    else
    {
        if( ( s[1] >= 'A' ) && ( s[1] <= 'F' ) )
        {
            lvaule = ( lvaule << 4 ) | ( s[1] - 'A' + 10 );
        }
        else if( ( s[1] >= 'a' ) && ( s[1] <= 'f' ) )
        {
            lvaule  = ( lvaule << 4 ) | ( s[1] - 'a' + 10 );
        }
    }
    return lvaule;
}

//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler for 4 parameters
//! representing an IP address and extracts the IP address defined by them.
//!
//! \param pcName is a pointer to a string containing the base name of the IP
//! address parameters.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//! \param pcValues is an array of values associated with each parameter from
//! the pcParam array.
//! \param pbError is a pointer that will be written to \b true if there is any
//! error during the parameter parsing process (parameter not found, value is
//! not a valid decimal number).
//!
//! This function searches an array of parameters to find four parameters
//! whose names are \e pcName appended with digits 1 - 4.  Each of these
//! parameters is expected to have a value which is a decimal number between
//! 0 and 255.  The parameter values are read and concatenated into an unsigned
//! long representing an IP address with parameter 1 in the leftmost postion.
//!
//! For example, if \e pcName points to string ``ip'', the function will look
//! for 4 CGI parameters named ``ip1'', ``ip2'', ``ip3'' and ``ip4'' and read
//! their values to generate an IP address of the form 0xAABBCCDD where ``AA''
//! is the value of parameter ``ip1'', ``BB'' is the value of ``p2'', ``CC''
//! is the value of ``ip3'' and ``DD'' is the value of ``ip4''.
//!
//! \return Returns the IP address read or 0 if an error is detected (in
//! which case \e *pbError will be \b true).
//
//*****************************************************************************
static tBoolean
ConfigGetCGIMacaddr( const char* pcToFind, int iNumParams, char* pcParam[],
                     char* pcValue[], char* pcDecoded,
                     unsigned long ulLen )
{
    int iParam;
    unsigned char i;
    unsigned char i1;
    unsigned char strtemp[MAX_MACSTRING_LEN];
    char* string_tmep;
    tBoolean berror;
    berror = false;
    i = 0;
    i1 = 0;
    //Query corresponding parameters and parameter values
    iParam = ConfigFindCGIParameter( pcToFind, pcParam, iNumParams );
    if( iParam != -1 )
    {
        //Take out the corresponding string
        //          ConfigDecodeFormString(pcValue[iParam],
        //                                 (char *)string_tmep,
        //                                 ulLen);
        string_tmep = pcValue[iParam];
        //String into an integer
        for( i = 0; i < 6; i++ )
        {
            i1 = 0;
            while( !( ( *string_tmep == '\x2d' ) || ( *string_tmep == 0 ) ) )
            {
                if( isxdigit( *string_tmep ) || ( *string_tmep == '\x2d' ) )
                {
                    strtemp[i1++] = *string_tmep++;
                }
                else
                {
                    berror = true;
                    return berror;
                }
            }
            strtemp[i1] = 0;
            string_tmep++;
            //sprintf(&pcDecoded[i],"%02X",(char*)strtemp);
            pcDecoded[i] = StringtoInt( ( char* )strtemp );
        }
        return berror;
    }
    berror = true;
    return berror;
}
//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/config.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/config.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``port'' indicates which connection's settings to update.  Valid
//!   values are ``0'' or ``1''.
//! - ``br'' supplies the baud rate.
//! - ``bc'' supplies the number of bits per character.
//! - ``parity'' supplies the parity.  Valid values are ``0'', ``1'', ``2'',
//!   ``3'' or ``4'' with meanings as defined by \b SERIAL_PARITY_xxx in
//!   serial.h.
//! - ``stop'' supplies the number of stop bits.
//! - ``flow'' supplies the flow control setting.  Valid values are ``1'' or
//!   ``3'' with meanings as defined by the \b SERIAL_FLOW_CONTROL_xxx in
//!   serial.h.
//! - ``telnetlp'' supplies the local port number for use by the telnet server.
//! - ``telnetrp'' supplies the remote port number for use by the telnet
//!   client.
//! - ``telnett'' supplies the telnet timeout in seconds.
//! - ``telnetip1'' supplies the first digit of the telnet server IP address.
//! - ``telnetip2'' supplies the second digit of the telnet server IP address.
//! - ``telnetip3'' supplies the third digit of the telnet server IP address.
//! - ``telnetip4'' supplies the fourth digit of the telnet server IP address.
//! - ``tnmode'' supplies the telnet mode, ``0'' for server, ``1'' for client.
//! - ``tnprot'' supplies the telnet protocol, ``0'' for telnet, ``1'' for raw.
//! - ``default'' will be defined with value ``1'' if the settings supplied are
//!   to be saved to flash as the defaults for this port.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char*
LoginCGIHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] )
{
    int iParam;
    long lpasswd;
    tBoolean bParamError;
    bParamError = false;
    char* rootpasswd = "root1234";
    //
    // Find the "modname" parameter.
    //
    iParam = ConfigFindCGIParameter( "PASSWD", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sWorkingDefaultParameters.userpasswd,
                                16 );
    }
    if( strncmp( ( char* )g_sWorkingDefaultParameters.userpasswd, rootpasswd, strlen( rootpasswd ) ) == 0 )
    {
        syslog = true;
        return( LOGIN_CGI_RESPONE1 );
    }
    else
    {
        syslog = false;
        return( LOGIN_CGI_RESPONE );
    }
}
static const char*
ConfigsetpwdCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                        char* pcValue[] )
{
    int iParam;
    //long lValue;
    tBoolean bChanged;
    //tBoolean bError;
    //
    // We have not made any changes that need written to flash yet.
    //
    bChanged = false;
    //
    // Find the "modname" parameter.
    //
    iParam = ConfigFindCGIParameter( "NEWPWD2", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sWorkingDefaultParameters.userpasswd,
                                16 );
        strncpy( ( char* )g_sParameters.userpasswd,
                 ( char* )g_sWorkingDefaultParameters.userpasswd, 16 );
        bChanged = true;
    }
    return( PASSWD_CGI_RESPONE );
}

static const char*
ConfigCTRStyeCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                         char* pcValue[] )
{
    tBoolean bError;
    unsigned char ulCtrStype;
    //
    //General the control stype from the cgi of http.
    //
    //
    // Find the "port" parameter.
    //
    bError = false;
    ulCtrStype = ConfigGetCGIParam( "ctrl", pcParam, pcValue, iNumParams, &bError );
    switch( ulCtrStype )
    {
        //
        // saved contrl stype .
        //
        case SAVED_CTR_STYPE:
            //save the current parameters and reset.
            g_sWorkingDefaultParameters = g_sParameters;
            ConfigSave();
            //set reset flag as tell the system to reset .
            // bReset = true;
            break;
        //
        // reset contrl stype .
        //
        case RESET_CTR_STYPE:
            //set reset flag as tell the system to reset .
            bReset = true;
            break;
        //
        // restore contrl stype .
        //
        case RESTORE_CTR_STYPE:
        {
            //
            // Update the working parameter set with the factory defaults.
            //
            ConfigLoadDeafault();
            //
            // Save the new defaults to flash.
            //
            ConfigSave();
            //
            // Apply the various changes required as a result of changing back to
            // the default settings.
            //
            ConfigUpdateAllParameters();
            //
            //set reset flag as tell the system to reset .
            //
            bReset = true;
        }
        break;
        //
        // redefault contrl stype .
        //
        case REDEFAULT_CTR_STYPE:
        {
            //
            // Update the working parameter set with the factory defaults.
            //
            ConfigLoadFactory();
            memcpy( &g_sWorkingDefaultParameters.ulMACAddr[0], &g_sParameters.ulMACAddr[0], 6 ); //MAC Address
            memcpy( g_sWorkingDefaultParameters.Serial, g_sParameters.Serial, 13 );
            //g_sSyscalparaters = g_FactorySyscalparameters;
            //
            // Save the new defaults to flash.
            //
            ConfigSave();
            //SystemSave();
            // Apply the various changes required as a result of changing back to
            // the default settings.
            //
            ConfigUpdateAllParameters();
            //
            //set reset flag as tell the system to reset .
            //
            bReset = true;
        }
        break;
    }
    return( CTRSTYPE_CGI_RESPONSE );
}
static const char*
ConfigresetCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                       char* pcValue[] )
{
    //
    //General the control stype from the cgi of http.
    //
    ConfigPreUpdateIPAddress();
    //
    // Find the "port" parameter.
    //
    g_sWorkingDefaultParameters.updata[0] = 'f';
    g_sWorkingDefaultParameters.updata[1] = 'a';
    g_sWorkingDefaultParameters.updata[2] = 'i';
    g_sWorkingDefaultParameters.updata[3] = 'l';
    ConfigSave();
    bReset   = true;
    return( UPD_CGI_RESONSE );
}
unsigned char b_zonechange = false;

extern u32_t   ACVOTAGE;

extern u32_t   ACLOADCUR;
static const char*
ConfigHomeHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] )
{
    // long lValue;
    tBoolean bParamError;         //cgi error .
    //tBoolean bRemoteCmdChanged;   //Remote cmd as ch1 to ch4 switch .
    unsigned char ch1RemoteCmd;   //Chnanel one remote command.
    unsigned char ch2RemoteCmd;   //Chnanel two remote command.
    unsigned char ch3RemoteCmd;   //Chnanel three remote command.
    unsigned char ch4RemoteCmd;   //Chnanel four remote command.
    tBoolean b_sysrefchanged;
    unsigned long tempdata;
    unsigned short vref;
    unsigned short iref;
    //
    // We have not encountered any parameter errors yet.
    //
    b_sysrefchanged = false;
    bParamError = false;
    if( syslog )
    {
        vref = ( unsigned short )ConfigGetCGIParam1( "vref", pcParam,
                pcValue,
                iNumParams,
                &bParamError );
        iref = ( unsigned short )ConfigGetCGIParam1( "iref", pcParam,
                pcValue,
                iNumParams,
                &bParamError );
        b_sysrefchanged = true;
    }
    //
    // the remote cgi command of the Channel one .
    //
    ch1RemoteCmd = ( unsigned long )ConfigGetCGIParam( "cm1", pcParam,
                   pcValue,
                   iNumParams,
                   &bParamError );
    //
    // the remote cgi command of the Channel two .
    //
    ch2RemoteCmd = ( unsigned char )ConfigGetCGIParam( "cm2", pcParam,
                   pcValue,
                   iNumParams,
                   &bParamError );
    //
    // the remote cgi command of the Channel three .
    //
    ch3RemoteCmd = ( unsigned long )ConfigGetCGIParam( "cm3", pcParam,
                   pcValue,
                   iNumParams,
                   &bParamError );
    //
    // the remote cgi command of the Channel four .
    //
    ch4RemoteCmd = ( unsigned long )ConfigGetCGIParam( "cm4", pcParam,
                   pcValue,
                   iNumParams,
                   &bParamError );
    //
    // We have now read all the parameters and made sure that they are valid
    // decimal numbers.  Did we see any errors during this process?
    //
    if( bParamError )
    {
        //
        // Yes - tell the user there was an error.
        //
        return( PARAM_ERROR_RESPONSE );
    }
    //
    //The remote command to set corresponding marks .
    //
    //   if(!(g_sParameters.remote_cmd & TIME_CH1_EN))
    //   {
    if( ch1RemoteCmd )
    {
        g_sParameters.remote_cmd |= CMD_CH1_OPEN;
    }
    else
    {
        g_sParameters.remote_cmd &= ~CMD_CH1_OPEN;
    }
    //   }
    //   if(!(g_sParameters.remote_cmd & TIME_CH2_EN))
    //   {
    if( ch2RemoteCmd )
    {
        g_sParameters.remote_cmd |= CMD_CH2_OPEN;
    }
    else
    {
        g_sParameters.remote_cmd &= ~CMD_CH2_OPEN;
    }
    //   }
    //   if(!(g_sParameters.remote_cmd & TIME_CH3_EN))
    //   {
    if( ch3RemoteCmd )
    {
        g_sParameters.remote_cmd |= CMD_CH3_OPEN;
    }
    else
    {
        g_sParameters.remote_cmd &= ~CMD_CH3_OPEN;
    }
    //   }
    //   if(!(g_sParameters.remote_cmd & TIME_CH4_EN))
    //   {
    if( ch4RemoteCmd )
    {
        g_sParameters.remote_cmd |= CMD_CH4_OPEN;
    }
    else
    {
        g_sParameters.remote_cmd &= ~CMD_CH4_OPEN;
    }
    //   }
    if( b_sysrefchanged )
    {
        g_sSyscalparaters.Vk = ACVOTAGE * 100 / vref;
        g_sSyscalparaters.Ik = ACLOADCUR * 100 / iref;
        //SystemSave();
    }
    //
    // Send the user back to the main status page.
    //
    if( syslog )
    {
        return( DEFAULT_CGI_RESPONSE1 );
    }
    else
    {
        return( DEFAULT_CGI_RESPONSE );
    }
}
static const char*
SNTPCGIHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] )
{
    int iParam;
    //long lValue;
    tBoolean bParamError;// /
    unsigned char ulSntpEn ;
    unsigned short ulSntpIntervatSecond;
    //
    // We have not encountered any parameter errors yet.
    //
    bParamError = false;
    //
    // SNTP Enable flag
    //
    ulSntpEn = ( unsigned char )ConfigGetCGIParam( "sntpen", pcParam,
               pcValue,
               iNumParams,
               &bParamError );
    //
    // SNTP URL/IP Adress.
    //
    iParam = ConfigFindCGIParameter( "ntpurl", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.SNTPURL,
                                URL_NAME_LEN );
        ugSntp_Changed = true ;
        //
        // Shut down connections in preparation for the IP address change.
        //
        ConfigPreUpdateIPAddress();
    }
    //
    //General the SNTP Intervate seconds from the http cgi.
    //
    bParamError = false;
    ulSntpIntervatSecond = ( unsigned short )ConfigGetCGIParam( "spinter", pcParam, pcValue, iNumParams,
                           &bParamError );
    //
    // We have now read all the parameters and made sure that they are valid
    // decimal numbers.  Did we see any errors during this process?
    //
    if( bParamError || ( ulSntpIntervatSecond <= SNTP_LOWER_INTER ) || ( ulSntpIntervatSecond >= SNTP_UPPER_INTER ) || ( ulSntpEn > SNTP_EN_UPPER_OPTION ) )
    {
        //
        // Yes - tell the user there was an error.
        //
        return( PARAM_ERROR_RESPONSE );
    }
    //    else
    //    {
    //        ugSntp_Changed = true;
    //    }
    //
    // Did any of the serial parameters change?
    //
    if( ulSntpEn )
    {
        g_sParameters.ucFlags |= SNTP_EN_FLAG;
    }
    else
    {
        g_sParameters.ucFlags &= ~SNTP_EN_FLAG;
        //
        // Shut down connections in preparation for the IP address change.
        //
        ConfigPreUpdateIPAddress();
        ugSntp_Changed = false;
    }
    g_sParameters.SNTPInterval = ulSntpIntervatSecond;
    if( ugSntp_Changed )
    {
        sntp_init();
    }
    //
    // Send the user back to the main status page.
    //
    return( SNTP_CGI_RESPONE );
}
static const char*
Config2CGIHandler1( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] )
{
    int iParam;
    tBoolean bParamError;
    unsigned short     ulOverVoltage;
    unsigned short     ulUVLO;
    unsigned short     ulOverCurrent;
    unsigned short     ulLeakCurrent;
    //
    // We have not encountered any parameter errors yet.
    //
    bParamError = false;
    //
    // General overvoltage from the cgi parameters of http.
    //
    ulOverVoltage = ( unsigned short )ConfigGetCGIParam1( "ovvol", pcParam,
                    pcValue,
                    iNumParams,
                    &bParamError );
    //
    // General UVLO from the cgi parameters of http.
    //
    ulUVLO = ( unsigned short )ConfigGetCGIParam1( "uvlow", pcParam,
             pcValue,
             iNumParams,
             &bParamError );
    //
    // General over current from the cgi parameters of http.
    //
    ulOverCurrent = ( unsigned short )ConfigGetCGIParam1( "overcur", pcParam,
                    pcValue,
                    iNumParams,
                    &bParamError );
    //
    // General the leakage current from the cgi parameters of http.
    //
    ulLeakCurrent = ( unsigned short )ConfigGetCGIParam1( "leakcur", pcParam,
                    pcValue,
                    iNumParams,
                    &bParamError );
    if( bParamError )
    {
        //
        // Yes - tell the user there was an error.
        //
        return( PARAM_ERROR_RESPONSE );
    }
    //
    // Did any of the module parameters change?
    //
    if( ( ulOverVoltage < VOL_UPPER_LIMIT ) && ( ulUVLO > VOL_LOWER_LIMIT ) && ( ulOverCurrent <= CUR_UPPER_LIMIT ) && ( ulLeakCurrent < LEAK_UPPER_LIMIT ) )
    {
        g_sParameters.OverVoltage = ulOverVoltage;
        g_sParameters.UVLO = ulUVLO;
        g_sParameters.OverCurrent = ulOverCurrent;
        g_sParameters.LeakCurrent = ulLeakCurrent;
    }
    //
    // Send the user back to the main status page.
    //
    return( CONFIG2_CGI_RESPONE );
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/ip.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/ip.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``staticip'' contains ``1'' to use a static IP address or ``0'' to use
//!   DHCP/AutoIP.
//! - ``sip1'' contains the first digit of the static IP address.
//! - ``sip2'' contains the second digit of the static IP address.
//! - ``sip3'' contains the third digit of the static IP address.
//! - ``sip4'' contains the fourth digit of the static IP address.
//! - ``gip1'' contains the first digit of the gateway IP address.
//! - ``gip2'' contains the second digit of the gateway IP address.
//! - ``gip3'' contains the third digit of the gateway IP address.
//! - ``gip4'' contains the fourth digit of the gateway IP address.
//! - ``mip1'' contains the first digit of the subnet mask.
//! - ``mip2'' contains the second digit of the subnet mask.
//! - ``mip3'' contains the third digit of the subnet mask.
//! - ``mip4'' contains the fourth digit of the subnet mask.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char*
ConfigIPCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                    char* pcValue[] )
{
    int iParam;
    long lValue;
    tBoolean bChanged;
    tBoolean bError;
    tBoolean bParamError;
    long lMode;
    unsigned long ulIPAddr;
    unsigned long ulGatewayAddr;
    unsigned long ulSubnetMask;
    unsigned long ulDNSIPAddr;
    unsigned char  ulMACAddr[6];
    unsigned short ulWebport;
    //
    // Nothing has changed and we have seen no errors so far.
    //
    bChanged = false;
    bParamError = false;
    ulIPAddr = 0;
    ulGatewayAddr = 0;
    ulSubnetMask = 0;
    //
    //General the mac address from the cgi of http.
    //
    if( syslog )
    {
        bParamError = ConfigGetCGIMacaddr( "macadd", iNumParams, pcParam, pcValue, ( char* )ulMACAddr, MAX_MACSTRING_LEN );
    }
    //
    // Get the IP selection mode.
    //
    lMode = ConfigGetCGIParam( "staticip", pcParam, pcValue, iNumParams,
                               &bParamError );
    //
    // This parameter is required so tell the user there has been a problem if
    // it is not found or is invalid.
    //
    if( bParamError )
    {
        return( PARAM_ERROR_RESPONSE );
    }
    //
    // If we are being told to use a static IP, read the remaining information.
    //
    if( lMode )
    {
        //
        // Get the static IP address to use.
        //
        ulIPAddr = ConfigGetCGIIPAddr( "sip", pcParam, pcValue, iNumParams,
                                       &bParamError );
        ulSubnetMask = ConfigGetCGIIPAddr( "mip", pcParam, pcValue, iNumParams,
                                           &bParamError );
        //
        // Get the gateway IP address to use.
        //
        ulGatewayAddr = ConfigGetCGIIPAddr( "gip", pcParam, pcValue, iNumParams,
                                            &bParamError );
    }
    //
    // Get the dns IP address to use.
    //
    ulDNSIPAddr  = ConfigGetCGIIPAddr( "dns", pcParam, pcValue, iNumParams,
                                       &bParamError );
    if( syslog )
    {
        //
        // Find the "wpt" parameter.
        //
        bError = false;
        ulWebport = ( unsigned short )ConfigGetCGIParam( "wpt", pcParam, pcValue, iNumParams,
                    &bError );
        if( !bError )
            if( ( ulWebport != g_sParameters.webport ) && ( ulWebport != g_sParameters.usLocationURLPort ) && ( ulWebport >= 80 ) && ( ulWebport <= 8080 ) )
            {
                g_sParameters.webport = ulWebport;
                g_sWorkingDefaultParameters = g_sParameters;
                ConfigSave();
                bReset   = true;
            }
        //              //
        //              // Find the "port" parameter.
        //              //
        //              bError = false;
        //              lValue = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bError);
        //              if(!bError)
        //              {
        //                      //
        //                      // The parameter was a valid decimal number.  If it is different
        //                      // from the current value, store it and note that we made a change
        //                      // that needs saving.
        //                      //
        //                      if((unsigned short int)lValue !=
        //                           g_sWorkingDefaultParameters.usLocationURLPort)
        //                      {
        //                              //
        //                              // Shut down UPnP temporarily.
        //                              //
        //                              //UPnPStop();
        //
        //                              //
        //                              // Update our working parameters and the default set.
        //                              //
        //                              g_sParameters.usLocationURLPort = (unsigned short)lValue;
        //                              g_sWorkingDefaultParameters.usLocationURLPort =
        //                                      (unsigned short int)lValue;
        //
        //                              //
        //                              // Restart UPnP with the new location port number.
        //                              //
        //                              //UPnPStart();
        //
        //                              //
        //                              // Remember that something changed.
        //                              //
        //                              bChanged = true;
        //                      }
        //              }
    }
    //
    // Make sure we read all the required parameters correctly.
    //
    if( bParamError )
    {
        //
        // Oops - some parameter was invalid.
        //
        return( PARAM_ERROR_RESPONSE );
    }
    //
    // We have all the parameters so determine if anything changed.
    //
    //
    // Did the basic mode change?
    //
    if( ( lMode && !( g_sParameters.ucFlags & CONFIG_FLAG_STATICIP ) ) ||
            ( !lMode && ( g_sParameters.ucFlags & CONFIG_FLAG_STATICIP ) ) )
    {
        //
        // The mode changed so set the new mode in the parameter block.
        //
        if( !lMode )
        {
            g_sParameters.ucFlags &= ~CONFIG_FLAG_STATICIP;
        }
        else
        {
            g_sParameters.ucFlags |= CONFIG_FLAG_STATICIP;
        }
        //
        // Remember that something changed.
        //
        bChanged = true;
    }
    //
    // If we are now using static IP, check for modifications to the IP
    // addresses and mask.
    //
    if( lMode )
    {
        if( ( g_sParameters.ulStaticIP != ulIPAddr ) ||
                ( g_sParameters.ulGatewayIP != ulGatewayAddr ) ||
                ( g_sParameters.ulSubnetMask != ulSubnetMask ) ||
                ( g_sParameters.ulDNSAddr != ulDNSIPAddr ) )
        {
            //
            // Something changed so update the parameter block.
            //
            bChanged = true;
            g_sParameters.ulStaticIP = ulIPAddr;
            g_sParameters.ulGatewayIP = ulGatewayAddr;
            g_sParameters.ulSubnetMask = ulSubnetMask;
            g_sParameters.ulDNSAddr = ulDNSIPAddr;
            g_sWorkingDefaultParameters = g_sParameters;
            ConfigSave();
        }
    }
    if( syslog )
    {
        memcpy( g_sParameters.ulMACAddr, ulMACAddr, 6 );
    }
    //
    // If anything changed, we need to resave the parameter block.
    //
    if( bChanged )
    {
        //
        // Shut down connections in preparation for the IP address change.
        //
        ConfigPreUpdateIPAddress();
        //
        // Update the working default set and save the parameter block.
        //
        //g_sWorkingDefaultParameters = g_sParameters;
        //ConfigSave();
        //
        // Tell the main loop that a IP address update has been requested.
        //
        g_bChangeIPAddress = true;
        //
        // Direct the browser to a page warning about the impending IP
        // address change.
        //
        return( IP_UPDATE_RESPONSE );
    }
    //
    // Direct the user back to our miscellaneous settings page.
    //
    return( MISC_PAGE_URI );
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/misc.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/misc.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``modname'' provides a string to be used as the friendly name for the
//!   module.  This is encoded by the browser and must be decoded here.
//! - ``port'' supplies TCP port to be used by UPnP.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char*
ConfigTrapCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                      char* pcValue[] )
{
    int iParam;
    long lValue;
    tBoolean bParamError;
    tBoolean bError;
    unsigned char ulTrapEn ;
    unsigned short ulTrapPort;
    //
    // We have not encountered any parameter errors yet.
    //
    bParamError = false;
    //
    // SNTP Enable flag
    //
    ulTrapEn = ( unsigned char )ConfigGetCGIParam( "trapen", pcParam,
               pcValue,
               iNumParams,
               &bParamError );
    //
    // SNTP URL/IP Adress.
    //
    iParam = ConfigFindCGIParameter( "trapip", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.TrapService,
                                URL_NAME_LEN );
    }
    //
    //General the SNTP Intervate seconds from the http cgi.
    //
    bError = false;
    ulTrapPort = ( unsigned short )ConfigGetCGIParam( "trapt", pcParam, pcValue, iNumParams,
                 &bError );
    //
    // We have now read all the parameters and made sure that they are valid
    // decimal numbers.  Did we see any errors during this process?
    //
    if( bParamError || ( ulTrapEn > TRAP_EN_UPPER_OPTION ) )
    {
        //
        // Yes - tell the user there was an error.
        //
        return( PARAM_ERROR_RESPONSE );
    }
    //
    // Did any of the serial parameters change?
    //
    if( ulTrapEn )
    {
        g_sParameters.ucFlags |= CONFIG_EN_TRAP;
    }
    else
    {
        g_sParameters.ucFlags &= ~CONFIG_EN_TRAP;
    }
    g_sParameters.TrapPort = ulTrapPort;
    //
    // Send the user back to the main status page.
    //
    return( TRAP_CGI_RESPONE );
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/defaults.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/defaults.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects no specific parameters and any passed are
//! ignored.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char*
ConfigRegateCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                        char* pcValue[] )
{
    tBoolean bError;
    unsigned char ulReCnt;
    unsigned short ulDelay1;
    unsigned short ulDelay2;
    unsigned short ulDelay3;
    //
    //General the  recount from the cgi of http.
    //
    bError = false;
    ulReCnt = ConfigGetCGIParam( "recnt", pcParam, pcValue, iNumParams, &bError );
    //
    //General the  first delay from the cgi of http.
    //
    bError = false;
    ulDelay1 = ConfigGetCGIParam( "rgdey1", pcParam, pcValue, iNumParams, &bError );
    //
    //General the  second delay from the cgi of http.
    //
    bError = false;
    ulDelay2 = ConfigGetCGIParam( "rgdey2", pcParam, pcValue, iNumParams, &bError );
    //
    //General the  third delay from the cgi of http.
    //
    bError = false;
    ulDelay3 = ConfigGetCGIParam( "rgdey3", pcParam, pcValue, iNumParams, &bError );
    //
    // Make sure we read all the required parameters correctly.
    //
    if( bError )
    {
        //
        // Oops - some parameter was invalid.
        //
        return( PARAM_ERROR_RESPONSE );
    }
    //
    //save the parameters into g_sParameters.
    //
    if( ulReCnt <= RECNT_UPPER_LIMIT )
    {
        g_sParameters.rgcnt = ulReCnt;
    }
    if( ( ulDelay1 >= REDEY_LOWER_LIMIT ) && ( ulDelay1 <= REDEY_UPPER_LIMIT ) )
    {
        g_sParameters.delay1 = ulDelay1;
    }
    if( ( ulDelay2 >= REDEY_LOWER_LIMIT ) && ( ulDelay2 <= REDEY_UPPER_LIMIT ) )
    {
        g_sParameters.delay2 = ulDelay2;
    }
    if( ( ulDelay3 >= REDEY_LOWER_LIMIT ) && ( ulDelay3 <= REDEY_UPPER_LIMIT ) )
    {
        g_sParameters.delay3 = ulDelay3;
    }
    return( RECNT_CGI_RESPONSE );
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/update.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/update.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects no parameters and ignores all passed.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char*
ConfigSwitchCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                        char* pcValue[] )
{
    long lValue;
    tBoolean bError;
    unsigned char iindex;
    unsigned char ulChannel1En;
    unsigned char ulChannel2En;
    unsigned char ulChannel3En;
    unsigned char ulChannel4En;
    tTimeParameters  ulBootTime[4];
    tTimeParameters  ulShutTime[4];
    //
    //General the swtich enable from the cgi of http.
    //
    bError = false;
    ulChannel1En = ConfigGetCGIParam( "swen1", pcParam, pcValue, iNumParams, &bError );
    //
    //General the hours of the boot time from the cgi of http.
    //
    bError = false;
    ulBootTime[0].hour = ConfigGetCGIParam( "ch1on1", pcParam, pcValue, iNumParams, &bError );
    //
    //General the minutes of the boot time from the cgi of http.
    //
    bError = false;
    ulBootTime[0].minute = ConfigGetCGIParam( "ch1on2", pcParam, pcValue, iNumParams, &bError );
    //
    //General the hours of the shut time from the cgi of http.
    //
    bError = false;
    ulShutTime[0].hour = ConfigGetCGIParam( "ch1on3", pcParam, pcValue, iNumParams, &bError );
    //
    //General the minutes of the shut time from the cgi of http.
    //
    bError = false;
    ulShutTime[0].minute = ConfigGetCGIParam( "ch1on4", pcParam, pcValue, iNumParams, &bError );
    /*********the parameters of the channel 2 time fixed ************/
    bError = false;
    ulChannel2En = ConfigGetCGIParam( "swen2", pcParam, pcValue, iNumParams, &bError );
    //
    //General the hours of the boot time from the cgi of http.
    //
    bError = false;
    ulBootTime[1].hour = ConfigGetCGIParam( "ch2on1", pcParam, pcValue, iNumParams, &bError );
    //
    //General the minutes of the boot time from the cgi of http.
    //
    bError = false;
    ulBootTime[1].minute = ConfigGetCGIParam( "ch2on2", pcParam, pcValue, iNumParams, &bError );
    //
    //General the hours of the shut time from the cgi of http.
    //
    bError = false;
    ulShutTime[1].hour = ConfigGetCGIParam( "ch2on3", pcParam, pcValue, iNumParams, &bError );
    //
    //General the minutes of the shut time from the cgi of http.
    //
    bError = false;
    ulShutTime[1].minute = ConfigGetCGIParam( "ch2on4", pcParam, pcValue, iNumParams, &bError );
    /*********the parameters of the channel 3 time fixed ************/
    bError = false;
    ulChannel3En = ConfigGetCGIParam( "swen3", pcParam, pcValue, iNumParams, &bError );
    //
    //General the hours of the boot time from the cgi of http.
    //
    bError = false;
    ulBootTime[2].hour = ConfigGetCGIParam( "ch3on1", pcParam, pcValue, iNumParams, &bError );
    //
    //General the minutes of the boot time from the cgi of http.
    //
    bError = false;
    ulBootTime[2].minute = ConfigGetCGIParam( "ch3on2", pcParam, pcValue, iNumParams, &bError );
    //
    //General the hours of the shut time from the cgi of http.
    //
    bError = false;
    ulShutTime[2].hour = ConfigGetCGIParam( "ch3on3", pcParam, pcValue, iNumParams, &bError );
    //
    //General the minutes of the shut time from the cgi of http.
    //
    bError = false;
    ulShutTime[2].minute = ConfigGetCGIParam( "ch3on4", pcParam, pcValue, iNumParams, &bError );
    /*********the parameters of the channel 4 time fixed ************/
    bError = false;
    ulChannel4En = ConfigGetCGIParam( "swen4", pcParam, pcValue, iNumParams, &bError );
    //
    //General the hours of the boot time from the cgi of http.
    //
    bError = false;
    ulBootTime[3].hour = ConfigGetCGIParam( "ch4on1", pcParam, pcValue, iNumParams, &bError );
    //
    //General the minutes of the boot time from the cgi of http.
    //
    bError = false;
    ulBootTime[3].minute = ConfigGetCGIParam( "ch4on2", pcParam, pcValue, iNumParams, &bError );
    //
    //General the hours of the shut time from the cgi of http.
    //
    bError = false;
    ulShutTime[3].hour = ConfigGetCGIParam( "ch4on3", pcParam, pcValue, iNumParams, &bError );
    //
    //General the minutes of the shut time from the cgi of http.
    //
    bError = false;
    ulShutTime[3].minute = ConfigGetCGIParam( "ch4on4", pcParam, pcValue, iNumParams, &bError );
    //
    //bError.
    //
    if( bError )
    {
        //
        // Yes - tell the user there was an error.
        //
        return( PARAM_ERROR_RESPONSE );
    }
    //
    //channel 1-4 time enable flag set.
    //
    if( ulChannel1En )
    {
        g_sParameters.remote_cmd |= imark[4] ;
    }
    else
    {
        g_sParameters.remote_cmd &= ~imark[4] ;
    }
    if( ulChannel2En )
    {
        g_sParameters.remote_cmd |= imark[5] ;
    }
    else
    {
        g_sParameters.remote_cmd &= ~imark[5] ;
    }
    if( ulChannel3En )
    {
        g_sParameters.remote_cmd |= imark[6] ;
    }
    else
    {
        g_sParameters.remote_cmd &= ~imark[6] ;
    }
    if( ulChannel4En )
    {
        g_sParameters.remote_cmd |= imark[7] ;
    }
    else
    {
        g_sParameters.remote_cmd &= ~imark[7] ;
    }
    //
    //save the parameters into g_sParameters.
    //
    for( iindex = 0; iindex < 4; iindex++ )
    {
        if( ( ulBootTime[iindex].hour < 24 ) && ( ulBootTime[iindex].minute < 60 ) )
        {
            g_sParameters.boottime[iindex] = ulBootTime[iindex];
        }
        if( ( ulShutTime[iindex].hour < 24 ) && ( ulShutTime[iindex].minute < 60 ) )
        {
            g_sParameters.shuttime[iindex] = ulShutTime[iindex];
        }
    }
    //
    //Respone the cgi html.
    //
    return( TFIX_CGI_RESPONE );
    //   bReset=true;
    //   return(DEFAULT_CGI_RESPONSE);
}
extern unsigned char keepalive_status;
extern unsigned char  Beathearfailcn;
extern unsigned link_status;
extern unsigned short Beathearstopcn;
extern unsigned short BeathearCNT;

static const char*
ConfigInfoCGIHandler( int iIndex, int iNumParams, char* pcParam[],
                      char* pcValue[] )
{
    int iParam;
    //long lValue;
    // tBoolean bError;
    //
    //General the mode name  from the cgi of http .
    //
    //
    // Find the "modname" parameter.
    //
    iParam = ConfigFindCGIParameter( "modname", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.ucModName,
                                MOD_NAME_LEN );
    }
    //
    // Find the "model" parameter.
    //
    iParam = ConfigFindCGIParameter( "model", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.Model,
                                DEVICE_INFO_LEN );
    }
    //
    // Find the "manu" parameter.
    //
    iParam = ConfigFindCGIParameter( "manu", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.Manufacturer,
                                DEVICE_INFO_LEN );
    }
    //
    // Find the "serial" parameter.
    //
    iParam = ConfigFindCGIParameter( "ser", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.Serial,
                                13 );
    }
    //
    // Find the "pos" parameter.
    //
    iParam = ConfigFindCGIParameter( "pos", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.InstallPos,
                                DEVICE_INFO_LEN );
    }
    //
    // Find the "instal" parameter.
    //
    iParam = ConfigFindCGIParameter( "instal", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.InstallPer,
                                DEVICE_INFO_LEN );
    }
    //
    // Find the "pos" parameter.
    //
    iParam = ConfigFindCGIParameter( "instime", pcParam, iNumParams );
    if( iParam != -1 )
    {
        ConfigDecodeFormString( pcValue[iParam],
                                ( char* )g_sParameters.InstallTime,
                                DEVICE_INFO_LEN );
    }
    return( INFO_CGI_RESONSE );
}
//
//clear the buffer,memset 0.
//
static  void  clear_response_bufer( unsigned char* buffer )
{
    memset( buffer, 0, strlen( ( const char* )buffer ) );
}
//
//！ clear sub buffer
//
static void clear_sub_buffer( unsigned char* buffer )
{
    memset( buffer, 0, strlen( ( const char* )buffer ) );
}
//
//!generate a string
//
static void Generate_String( u8_t quene, unsigned char* buffer )
{
    unsigned char i = 0;
    clear_sub_buffer( buffer );
    switch( quene )
    {
        case XML_SYSVOL_STRING:
        {
            //          if(syslog)
            //              usnprintf(buffer,XML_TMPBUF_SIZE,"<input type='text' value='%d.%02d' name='vref' maxlength='6' size='8'>",analogValue[0]/100,analogValue[0]%100);
            //          else
            usnprintf( buffer, XML_TMPBUF_SIZE, "%d.%02d", analogValue[0] / 100, analogValue[0] % 100 );
        }
        break;
        case XML_SYSCUR_STRING:
        {
            //        if(syslog)
            //             usnprintf(buffer,XML_TMPBUF_SIZE,"<input type='text' value='%d.%02d'name='iref' maxlength='6' size='8'>",analogValue[2]/100,analogValue[2]%100);
            //        else
            usnprintf( buffer, XML_TMPBUF_SIZE, "%d.%02d", analogValue[2] / 100, analogValue[2] % 100 );
        }
        break;
        case XML_SYSTMP_STRING:
        {
            usnprintf( buffer, XML_TMPBUF_SIZE, "%d.%d", temp_value / 10, temp_value % 10 );
        }
        break;
        case XML_SYSHUI_STRING:
        {
            usnprintf( buffer, XML_TMPBUF_SIZE, "%d.%d", humi_value / 10, humi_value % 10 );
        }
        break;
        case XML_CH1STA_STRING:
        case XML_CH2STA_STRING:
        case XML_CH3STA_STRING:
        case XML_CH4STA_STRING:
        {
            i = quene - XML_CH1STA_STRING;
            if( ChannelStatus & imark[i] )
            {
                usnprintf( ( char* )buffer, XML_TMPBUF_SIZE, "A" );
            }
            else
            {
                usnprintf( ( char* )buffer, XML_TMPBUF_SIZE, "B" );
            }
        }
        break;
        case XML_CH1RSN_STRING:
        case XML_CH2RSN_STRING:
        case XML_CH3RSN_STRING:
        case XML_CH4RSN_STRING:
        {
            i = quene - XML_CH1RSN_STRING;
            if( SysAlarm_Flag & HIGH_RISK_BIT )
            {
                usnprintf( buffer, XML_TMPBUF_SIZE, "C" );
            }
            else if( g_sParameters.remote_cmd & imark[i] )
            {
                usnprintf( buffer, XML_TMPBUF_SIZE, "A" );
            }
            else
            {
                usnprintf( buffer, XML_TMPBUF_SIZE, "B" );
            }
        }
        break;
        default:
            break;
    }
}
//
//! insert to the RESPONE
//
static void Insert2XML_Respone_String( void )
{
    unsigned char uLoopCnt = 0;
    unsigned char uTempBuf[XML_TMPBUF_SIZE];
    for( uLoopCnt = 0; uLoopCnt <= XML_CH4RSN_STRING; uLoopCnt++ )
    {
        Generate_String( uLoopCnt, uTempBuf );
        strcat( ( char* )( data_response_buf ), uTempBuf );
        strcat( ( char* )( data_response_buf ), ";" );
    }
}
//
//xml respone .
//
static const char* Orther_CGIHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] )
{
    clear_response_bufer( data_response_buf );
    Insert2XML_Respone_String();
    return RESPONSE_PAGE_SET_CGI_RSP_URL;
}
//
//xml passwrd respone.
//
static const char* Opther_CGIHandler( int iIndex, int iNumParams, char* pcParam[], char* pcValue[] )
{
    char buf[32];
    clear_response_bufer( data_response_buf );
    memset( buf, 0, sizeof( buf ) );
    sprintf( buf, "%s;root1234;", g_sParameters.userpasswd );
    strcat( ( char* )( data_response_buf ), buf );
    return RESPONSE_PAGE_SET_CGI_RSP_URL;
}
//*****************************************************************************
//! \internal
//!
//! Provides replacement text for each of our configured SSI tags.
//!
//! \param iIndex is an index into the g_pcConfigSSITags array and indicates
//! which tag we are being passed
//! \param pcInsert points to a buffer into which this function should write
//! the replacement text for the tag.  This should be plain text or valid HTML
//! markup.
//! \param iInsertLen is the number of bytes available in the pcInsert buffer.
//! This function must ensure that it does not write more than this or memory
//! corruption will occur.
//!
//! This function is called by the HTTPD server whenever it is serving a page
//! with a ``.ssi'', ``.shtml'' or ``.shtm'' file extension and encounters a
//! tag of the form <!--#tagname--> where ``tagname'' is found in the
//! g_pcConfigSSITags array.  The function writes suitable replacement text to
//! the \e pcInsert buffer.
//!
//! \return Returns the number of bytes written to the pcInsert buffer, not
//! including any terminating NULL.
//
//*****************************************************************************
unsigned char page_en = 0;
static uint16_t
ConfigSSIHandler( int iIndex, char* pcInsert, int iInsertLen )
{
    //unsigned long ulPort;
    int iCount;
    //
    // Which SSI tag are we being asked to provide content for?
    //
    switch( iIndex )
    {
        //
        // The local IP address tag "ipaddr".
        case SSI_INDEX_OLDPID:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input type='Hidden' value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    ConfigEncodeFormString( ( char* )g_sParameters.userpasswd,
                                            pcInsert + iCount,
                                            iInsertLen - iCount );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' maxlength='%d' size='%d' name='oldpid'>",
                               ( 9 ), 10 );
            }
            return( iCount );
        }
        case SSI_INDEX_NOPID:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input type='Hidden' value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    ConfigEncodeFormString( ( char* )g_sParameters.userpasswd,
                                            pcInsert + iCount,
                                            iInsertLen - iCount );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' maxlength='%d' size='%d' name='NOpid'>",
                               ( 9 ), 10 );
            }
            return( iCount );
        }
        case SSI_INDEX_SYSVOL:
        {
            if( syslog )
            {
                iCount = usnprintf( pcInsert, iInsertLen, "<input type='text' value='" );
                if( iCount < iInsertLen )
                {
                    iCount +=
                        usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d.%02d",
                                   analogValue[0] / 100, analogValue[0] % 100 );
                }
                if( iCount < iInsertLen )
                {
                    iCount +=
                        usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                   "' name='vref' maxlength='%d' size='%d'>", 6, 8 );
                }
                return( iCount );
            }
            else
                return( usnprintf( pcInsert, iInsertLen, "%d.%02d V,",
                                   analogValue[0] / 100,
                                   analogValue[0] % 100 ) );
        }
        case SSI_INDEX_SYSCUR:
        {
            if( syslog )
            {
                iCount = usnprintf( pcInsert, iInsertLen, "<input type='text' value='" );
                if( iCount < iInsertLen )
                {
                    iCount +=
                        usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d.%02d",
                                   analogValue[2] / 100, analogValue[2] % 100 );
                }
                if( iCount < iInsertLen )
                {
                    iCount +=
                        usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                   "' name='iref' maxlength='%d' size='%d'>", 6, 8 );
                }
                return( iCount );
            }
            else
                return( usnprintf( pcInsert, iInsertLen, "%d.%02d A.",
                                   analogValue[2] / 100,
                                   analogValue[2] % 100 ) );
        }
        case SSI_INDEX_TEMPVAR:
        {
            return( usnprintf( pcInsert, iInsertLen, "%d.%d",
                               temp_value / 10,
                               temp_value % 10 ) );
        }
        case SSI_INDEX_HUMIVAR:
        {
            return( usnprintf( pcInsert, iInsertLen, "%d.%d",
                               humi_value / 10,
                               humi_value % 10 ) );
        }
        case SSI_INDEX_CTRVARS:
        {
            unsigned char command[4];
            command[0] = ( g_sParameters.remote_cmd & 0x01 ) ? 1 : 0;
            command[1] = ( g_sParameters.remote_cmd & 0x02 ) ? 1 : 0;
            command[2] = ( g_sParameters.remote_cmd & 0x04 ) ? 1 : 0;
            command[3] = ( g_sParameters.remote_cmd & 0x08 ) ? 1 : 0;
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     CTR_JAVASCRIPT_VARS,
                                     command[0],
                                     command[1],
                                     command[2],
                                     command[3]
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        case SSI_INDEX_STATUS1:
        case SSI_INDEX_STATUS2:
        case SSI_INDEX_STATUS3:
        case SSI_INDEX_STATUS4:
        {
            unsigned char aindex = ( iIndex - SSI_INDEX_STATUS1 );
            iCount = usnprintf( pcInsert, iInsertLen, "<font color='" );
            if( iCount < iInsertLen )
            {
                if( ChannelStatus & imark[aindex] )
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "green'>线路开</font>" );
                }
                else
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "red'>线路关</font>" );
                }
            }
            return( iCount );
        }
        case SSI_INDEX_LAMP1:
        case SSI_INDEX_LAMP2:
        case SSI_INDEX_LAMP3:
        case SSI_INDEX_LAMP4:
        {
            unsigned char aindex = ( iIndex - SSI_INDEX_LAMP1 );
            iCount = usnprintf( pcInsert, iInsertLen, "<font color='" );
            if( iCount < iInsertLen )
            {
                if( g_sParameters.remote_cmd & imark[aindex] )
                {
                    if( ChannelStatus & imark[aindex] )
                    {
                        iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "green'>远程命令开</font>" );
                    }
                    else
                    {
                        iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "red'>报警</font>" );
                    }
                }
                else
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "red'>远程命令关</font>" );
                }
            }
            return( iCount );
        }
        case SSI_INDEX_UNEARTH:
        case SSI_INDEX_LNORDER:
        case SSI_INDEX_LPROFAIL:
        case SSI_INDEX_OVCURRA:
        case SSI_INDEX_OVVOLA:
        case SSI_INDEX_UVLOA:
        case SSI_INDEX_LEAKCA:
        case SSI_INDEX_INVCH1:
        case SSI_INDEX_INVCH2:
        case SSI_INDEX_INVCH3:
        case SSI_INDEX_INVCH4:
        {
            unsigned char aindex = ( iIndex - SSI_INDEX_UNEARTH );
            iCount = usnprintf( pcInsert, iInsertLen, "<font color='" );
            if( iCount < iInsertLen )
            {
                if( SysAlarm_Flag & imark[aindex] )
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "red'>报警</font>" );
                }
                else
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "green'>无报警</font>" );
                }
            }
            return( iCount );
        }
        case SSI_INDEX_LAMP:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<font color='" );
            if( iCount < iInsertLen )
            {
                if( !discreteValue[7] )
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "red'>报警</font>" );
                }
                else
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "green'>无报警</font>" );
                }
            }
            return( iCount );
        }
        case SSI_INDEX_OVERVOL:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input type='text' value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d.%02d",
                               g_sParameters.OverVoltage / 100, g_sParameters.OverVoltage % 100 );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' name='ovvol' maxlength='%d' size='%d'>", 6, 10 );
            }
            return( iCount );
        }
        case SSI_INDEX_UVLO:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input type='text' value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d.%02d",
                               g_sParameters.UVLO / 100, g_sParameters.UVLO % 100 );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' name='uvlow' maxlength='%d' size='%d'>", 6, 10 );
            }
            return( iCount );
        }
        case SSI_INDEX_OVERCUR:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input type='text' value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d.%02d",
                               g_sParameters.OverCurrent / 100, g_sParameters.OverCurrent % 100 );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' name='overcur' maxlength='%d' size='%d'>", 5, 10 );
            }
            return( iCount );
        }
        case SSI_INDEX_LEAKCUR:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input type='text' value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d.%02d",
                               g_sParameters.LeakCurrent / 100, g_sParameters.LeakCurrent % 100 );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' name='leakcur' maxlength='%d' size='%d'>", 5, 10 );
            }
            return( iCount );
        }
        //
        //! IP Config Vars
        //
        //
        // The local MAC address tag "macaddr".
        //
        case SSI_INDEX_MACVARS:
        {
            if( syslog )
            {
                iCount = usnprintf( pcInsert, iInsertLen, "<input type='text' value='" );
                if( iCount < iInsertLen )
                {
                    iCount +=
                        usnprintf( pcInsert + iCount, iInsertLen - iCount, "%02X-%02X-%02X-%02X-%02X-%02X",
                                   g_sParameters.ulMACAddr[0],
                                   g_sParameters.ulMACAddr[1],
                                   g_sParameters.ulMACAddr[2],
                                   g_sParameters.ulMACAddr[3],
                                   g_sParameters.ulMACAddr[4],
                                   g_sParameters.ulMACAddr[5] );
                }
                if( iCount < iInsertLen )
                {
                    iCount +=
                        usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                   "' name='macadd' maxlength='%d' size='%d'>", MAX_MACSTRING_LEN, MAX_MACSTRING_LEN );
                }
                return( iCount );
            }
            else
                return( usnprintf( pcInsert, iInsertLen, "%02X-%02X-%02X-%02X-%02X-%02X",
                                   g_sParameters.ulMACAddr[0],
                                   g_sParameters.ulMACAddr[1],
                                   g_sParameters.ulMACAddr[2],
                                   g_sParameters.ulMACAddr[3],
                                   g_sParameters.ulMACAddr[4],
                                   g_sParameters.ulMACAddr[5] ) );
        }
        case SSI_INDEX_IPVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     IP_JAVASCRIPT_VARS,
                                     ( g_sParameters.ucFlags &
                                       CONFIG_FLAG_STATICIP ) ? 1 : 0,
                                     ( g_sParameters.ulStaticIP >> 24 ) & 0xFF,
                                     ( g_sParameters.ulStaticIP >> 16 ) & 0xFF,
                                     ( g_sParameters.ulStaticIP >> 8 ) & 0xFF,
                                     ( g_sParameters.ulStaticIP >> 0 ) & 0xFF );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        case SSI_INDEX_SNVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     SUBNET_JAVASCRIPT_VARS,
                                     g_sParameters.ucSyslog,
                                     ( g_sParameters.ulSubnetMask >> 24 ) & 0xFF,
                                     ( g_sParameters.ulSubnetMask >> 16 ) & 0xFF,
                                     ( g_sParameters.ulSubnetMask >> 8 ) & 0xFF,
                                     ( g_sParameters.ulSubnetMask >> 0 ) & 0xFF );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        // Generate a block of JavaScript variables containing the current
        // subnet mask.
        //
        case SSI_INDEX_GWVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     GW_JAVASCRIPT_VARS,
                                     ( g_sParameters.ulGatewayIP >> 24 ) & 0xFF,
                                     ( g_sParameters.ulGatewayIP >> 16 ) & 0xFF,
                                     ( g_sParameters.ulGatewayIP >> 8 ) & 0xFF,
                                     ( g_sParameters.ulGatewayIP >> 0 ) & 0xFF );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current DNS
        // servicer IP adress.
        //
        case SSI_INDEX_DNSVAR:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     DNS_JAVASCRIPT_VARS,
                                     ( g_sParameters.ulDNSAddr >> 24 ) & 0xFF,
                                     ( g_sParameters.ulDNSAddr >> 16 ) & 0xFF,
                                     ( g_sParameters.ulDNSAddr >> 8 ) & 0xFF,
                                     ( g_sParameters.ulDNSAddr >> 0 ) & 0xFF,
                                     g_sParameters.webport );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        case SSI_INDEX_WEBPORT:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input type='text' value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d",
                               g_sParameters.webport );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' name='wpt' maxlength='%d' size='%d'>", 5, 10 );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current UPnP port
        // number.
        //
        case SSI_INDEX_PNPINP:
        {
            return( usnprintf( pcInsert, iInsertLen,
                               "%d", g_sParameters.usLocationURLPort ) );
        }
        //
        // Generate an HTML text input field containing the current Sntp enable/disable.
        //
        //
        case SSI_INDEX_SNTPVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     SNTP_JAVASCRIPT_VARS,
                                     ( g_sParameters.ucFlags &
                                       CONFIG_EN_SNTP ) ? 1 : 0 );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current SNTP service IP address.
        //
        //
        case SSI_INDEX_SNTPURL:
        {
            struct ip_addr ucSntpServiceIP;
            char* pucurl = ( char* )g_sParameters.SNTPURL;
            ucSntpServiceIP.addr = inet_addr( ( char* )pucurl );
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( ucSntpServiceIP.addr != INADDR_NONE )
            {
                if( iCount < iInsertLen )
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d.%d.%d.%d",
                                         ( ( ucSntpServiceIP.addr >>  0 ) & 0xFF ),
                                         ( ( ucSntpServiceIP.addr >>  8 ) & 0xFF ),
                                         ( ( ucSntpServiceIP.addr >> 16 ) & 0xFF ),
                                         ( ( ucSntpServiceIP.addr >> 24 ) & 0xFF ) );
                }
            }
            else
            {
                if( iCount < iInsertLen )
                {
                    iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "%s", pucurl );
                }
            }
            iCount +=
                usnprintf( pcInsert + iCount, iInsertLen - iCount,
                           "' maxlength='%d' size='%d' name='ntpurl'>",
                           ( URL_NAME_LEN - 1 ), 40 );
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current SNTP interval seconds.
        //
        //
        case SSI_INDEX_SNTPINTER:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount, "%d",
                                     g_sParameters.SNTPInterval );
            }
            iCount +=
                usnprintf( pcInsert + iCount, iInsertLen - iCount,
                           "' maxlength='%d' size='%d' name='spinter'>",
                           ( URL_NAME_LEN - 1 ), 40 );
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current snmp trap enable.
        //
        //
        case SSI_INDEX_TRAPVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     SNMP_JAVASCRIPT_VARS,
                                     ( g_sParameters.ucFlags &
                                       CONFIG_EN_TRAP ) ? 1 : 0 );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        //
        // SNMP trap ip address.
        //
        case SSI_INDEX_TRAPIP:
        {
            struct ip_addr ucTrapServiceIP;
            char* pucurl = ( char* )g_sParameters.TrapService;
            ucTrapServiceIP.addr = inet_addr( ( char* )pucurl );
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%d.%d.%d.%d",
                                     ( ucTrapServiceIP.addr >> 0 ) & 0xff,
                                     ( ucTrapServiceIP.addr >> 8 ) & 0xff,
                                     ( ucTrapServiceIP.addr >> 16 ) & 0xff,
                                     ( ucTrapServiceIP.addr >> 24 ) & 0xff
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "' maxlength='15' size='15' name='trapip'>" );
            }
            return( iCount );
        }
        //
        //
        // SNMP trap port.
        //
        case SSI_INDEX_TRAPPORT:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%d",
                                     g_sParameters.TrapPort
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "' maxlength='5' size='15' name='trapt'>" );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current swtith fixed time enable.
        //
        //
        case SSI_INDEX_SWENVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     SWITCH_JAVASCRIPT_VARS,
                                     ( ( g_sParameters.remote_cmd & imark[4] ) ? 1 : 0 ),
                                     ( ( g_sParameters.remote_cmd & imark[5] ) ? 1 : 0 ),
                                     ( ( g_sParameters.remote_cmd & imark[6] ) ? 1 : 0 ),
                                     ( ( g_sParameters.remote_cmd & imark[7] ) ? 1 : 0 )
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current hours of swtith boot time.
        //
        //
        case SSI_INDEX_BOOTHVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     BOOTH_JAVASCRIPT_VARS,
                                     ( g_sParameters.boottime[0].hour ),
                                     ( g_sParameters.boottime[1].hour ),
                                     ( g_sParameters.boottime[2].hour ),
                                     ( g_sParameters.boottime[3].hour )
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current hours of swtith boot time.
        //
        //
        case SSI_INDEX_BOOTMVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     BOOTM_JAVASCRIPT_VARS,
                                     ( g_sParameters.boottime[0].minute ),
                                     ( g_sParameters.boottime[1].minute ),
                                     ( g_sParameters.boottime[2].minute ),
                                     ( g_sParameters.boottime[3].minute )
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current hours of swtith shut time.
        //
        //
        case SSI_INDEX_SHUTHVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     SHUTH_JAVASCRIPT_VARS,
                                     ( g_sParameters.shuttime[0].hour ),
                                     ( g_sParameters.shuttime[1].hour ),
                                     ( g_sParameters.shuttime[2].hour ),
                                     ( g_sParameters.shuttime[3].hour )
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current hours of swtith shut time.
        //
        //
        case SSI_INDEX_SHUTMVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     SHUTM_JAVASCRIPT_VARS,
                                     ( g_sParameters.shuttime[0].minute ),
                                     ( g_sParameters.shuttime[1].minute ),
                                     ( g_sParameters.shuttime[2].minute ),
                                     ( g_sParameters.shuttime[3].minute )
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing the current regate count.
        //
        //
        case SSI_INDEX_RGVARS:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     REGATE_JAVASCRIPT_VARS,
                                     g_sParameters.rgcnt );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s", JAVASCRIPT_FOOTER );
            }
            return( iCount );
        }
        //
        //
        // regate delay1 first.
        //
        case SSI_INDEX_DELAY1:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%d",
                                     g_sParameters.delay1
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "' maxlength='5' size='15' name='rgdey1'>" );
            }
            return( iCount );
        }
        //
        //
        // regate delay1 second.
        //
        case SSI_INDEX_DELAY2:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%d",
                                     g_sParameters.delay2
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "' maxlength='5' size='15' name='rgdey2'>" );
            }
            return( iCount );
        }
        //
        //
        // regate delay3 second.
        //
        case SSI_INDEX_DELAY3:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%d",
                                     g_sParameters.delay3
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "' maxlength='5' size='15' name='rgdey3'>" );
            }
            return( iCount );
        }
        //
        // Return the user-editable friendly name for the module.
        //
        case SSI_INDEX_MODENAME:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    ConfigEncodeFormString( ( char* )g_sParameters.ucModName,
                                            pcInsert + iCount,
                                            iInsertLen - iCount );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' maxlength='%d' size='%d' name='modname'>",
                               ( MOD_NAME_LEN - 1 ), MOD_NAME_LEN );
            }
            return( iCount );
        }
        //
        // Return the user-editable model name for the module.
        //
        case SSI_INDEX_MODEL:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    ConfigEncodeFormString( ( char* )g_sParameters.Model,
                                            pcInsert + iCount,
                                            iInsertLen - iCount );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' maxlength='%d' size='%d' name='model'>",
                               ( DEVICE_INFO_LEN - 1 ), DEVICE_INFO_LEN );
            }
            return( iCount );
        }
        //
        // Return the user-editable friendly manufacture for the module.
        //
        case SSI_INDEX_MANUFACT:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    ConfigEncodeFormString( ( char* )g_sParameters.Manufacturer,
                                            pcInsert + iCount,
                                            iInsertLen - iCount );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' maxlength='%d' size='%d' name='manu'>",
                               ( DEVICE_INFO_LEN - 1 ), DEVICE_INFO_LEN );
            }
            return( iCount );
        }
        //
        // Generate an HTML text input field containing serial
        // number.
        //
        case SSI_INDEX_SERIAL:
        {
            return( usnprintf( pcInsert, iInsertLen,
                               "<input value='%s' maxlength='12' size='30' "
                               "name='ser'>", g_sParameters.Serial ) );
        }
        //
        // Return the user-editable friendly install position for the module.
        //
        case SSI_INDEX_POSITION:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    ConfigEncodeFormString( ( char* )g_sParameters.InstallPos,
                                            pcInsert + iCount,
                                            iInsertLen - iCount );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' maxlength='%d' size='%d' name='pos'>",
                               ( DEVICE_INFO_LEN - 1 ), DEVICE_INFO_LEN );
            }
            return( iCount );
        }
        //
        // Return the user-editable friendly install persion for the module.
        //
        case SSI_INDEX_INSTALPERSON:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount +=
                    ConfigEncodeFormString( ( char* )g_sParameters.InstallPer,
                                            pcInsert + iCount,
                                            iInsertLen - iCount );
            }
            if( iCount < iInsertLen )
            {
                iCount +=
                    usnprintf( pcInsert + iCount, iInsertLen - iCount,
                               "' maxlength='%d' size='%d' name='insper'>",
                               ( DEVICE_INFO_LEN - 1 ), DEVICE_INFO_LEN );
            }
            return( iCount );
        }
        //
        // The local MAC address tag "macaddr".
        //
        case SSI_INDEX_INSTALTIME:
        {
            iCount = usnprintf( pcInsert, iInsertLen, "<input value='" );
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "%s",
                                     g_sParameters.InstallTime
                                   );
            }
            if( iCount < iInsertLen )
            {
                iCount += usnprintf( pcInsert + iCount, iInsertLen - iCount,
                                     "' maxlength='19' size='30' name='instime'>" );
            }
            return( iCount );
        }
        //
        // The run time.
        //
        case SSI_INDEX_RUNTIME://
        {
            unsigned long ucRunDay = g_sParameters.RunSecond / 86400;
            unsigned long ucRunHour = g_sParameters.RunSecond % 86400 / 3600;
            unsigned long ucRunMinute = g_sParameters.RunSecond % 86400 % 3600 / 60;
            unsigned long ucRunSecond = g_sParameters.RunSecond % 86400 % 3600 % 60;
            if( old_SysRunDay != ucRunDay )
            {
                g_sWorkingDefaultParameters = g_sParameters;
                ConfigSave();
            }
            old_SysRunDay = ucRunDay;
            return( usnprintf( pcInsert, iInsertLen, "%d day %d:%d:%d",
                               ucRunDay, ucRunHour, ucRunMinute, ucRunSecond ) );
        }
        //
        // The Firmware Version number tag, "revision".
        //
        case SSI_INDEX_SOFTVER://
        {
            return( usnprintf( pcInsert, iInsertLen, "%s",
                               "V1.0" ) );
        }
        //
        // The Firmware Version number tag, "revision".
        //
        case SSI_INDEX_HARDVER://
        {
            return( usnprintf( pcInsert, iInsertLen, "%s",
                               "T1.0" ) );
        }
        case SSI_INDEX_CURTIME:
        {
            return( usnprintf( pcInsert, iInsertLen, "%d-%d-%d  %d:%d:%d",
                               CurrentTime.year,
                               CurrentTime.month,
                               CurrentTime.day,
                               CurrentTime.hour,
                               CurrentTime.minute,
                               CurrentTime.second ) );
        }
        //
        // All other tags are unknown.
        //
        default:
        {
            return( usnprintf( pcInsert, iInsertLen,
                               "<b><i>Tag %d unknown!</i></b>", iIndex ) );
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************