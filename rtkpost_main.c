/*------------------------------------------------------------------------------
* rtkpost_main.c : RTKLIB rtkpost 解算引擎 —— 命令行驱动程序（带参数标注）
*
* 用途 : 读取 RINEX 观测/导航文件（可选精密星历 .sp3/.clk），调用 RTKLIB
*        后处理解算引擎 postpos()（postpos.c），逐历元执行
*        pntpos()/relpos()/pppos()（rtkpos.c），输出流动站坐标解算结果。
*
* 编译（Windows + MinGW-W64 gcc，本机已安装）:
*     build.bat            ← 双击或命令行执行，生成 rtkpost_main.exe
*     或  mingw32-make -f Makefile
*
* 运行示例:
*     动态RTK:  rtkpost_main.exe -p 2 -v 3.0 rover.obs base.obs nav.nav
*     静态RTK:  rtkpost_main.exe -p 3 -e rover.obs base.obs nav.nav -o res.pos
*     单点定位: rtkpost_main.exe -p 0 rover.obs nav.nav
*     前后向结合: rtkpost_main.exe -p 2 -c rover.obs base.obs nav.nav
*     (文件顺序: 第1个=流动站观测, 第2个=基准站观测(RTK用), 其余=导航/星历)
*
* 参数标注说明:
*     ◇需设置  使用前必须设置的数据（不改会出错或没有意义）
*     △可更改  有默认值、可按需调整的可调参数（高级选项）
*     ○自动    程序自动计算/从文件读取，一般无需手动设置
*-----------------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rtklib.h"

#define PROGNAME    "rtkpost_main"
#define MAXFILE     16

/* 消息显示回调（postpos.c 里 showmsg() 调用来显示进度，必须提供）------------*/
extern int showmsg(const char *format, ...)
{
    va_list arg;
    va_start(arg,format); vfprintf(stderr,format,arg); va_end(arg);
    fprintf(stderr,"\r");
    return 0;
}
/* 时间跨度回调（进度显示用，命令行版置空即可） ------------------------------*/
extern void settspan(gtime_t ts, gtime_t te) {}
extern void settime(gtime_t time) {}

/* Windows路径规范化: RTKLIB的expath()只用反斜杠'\'提取目录,正斜杠'/'路径
 * 会被截断成纯文件名(open error)。统一把输入/输出路径的'/'转成'\'。---------*/
static void topath(char *p)
{
    for (;*p;p++) if (*p=='/') *p='\\';
}

/* 帮助文本 ------------------------------------------------------------------*/
static void printhelp(void)
{
    fprintf(stderr,
" usage: rtkpost_main [option]... file file [...]\n"
" Read RINEX OBS/NAV/GNAV/HNAV/CLK, SP3 files, compute rover position and\n"
" output solution. The 1st RINEX OBS = rover, the 2nd OBS = base station,\n"
" the rest = navigation / precise ephemeris files.\n"
" [option]:\n"
"  -p mode   定位模式 0单点/1DGPS/2动态RTK/3静态RTK/4动基线/5固定/6PPP动态/7PPP静态 [2]\n"
"  -m mask   高度角掩蔽角(deg) [15]\n"
"  -f freq   频率数 1:L1 2:L1+L2 3:L1+L2+L5 [2]\n"
"  -sys s    卫星系统 s=G/R/E/J/C/I，可连续或逗号分隔，如 GEC 或 G,E,C [G,R]\n"
"  -iono n   电离层 0关/1广播/2SBAS/3消电离层LC/4估计/5IONEX [1(广播模型)]\n"
"  -trop n   对流层 0关/1Saastamoinen/2SBAS/3ZTD估计/4ZTD+梯度 [1]\n"
"  -v thres  整周模糊度ratio检验阈值(0.0=不做AR) [3.0]\n"
"  -i        瞬时模糊度固定(INST)\n"
"  -h        固定并保持模糊度(FIXHOLD)\n"
"  -c        前后向结合解算(推荐,更平滑) [前向]\n"
"  -b        后向解算 [前向]\n"
"  -r x y z  基准站坐标ECEF(m)\n"
"  -l lat lon hgt  基准站坐标经纬高(deg/m)\n"
"  -ts ds ts 开始日期/时刻(y/m/d h:m:s) [观测起点]\n"
"  -te de te 结束日期/时刻 [观测终点]\n"
"  -ti tint  解算采样间隔(s) [全部]\n"
"  -o file   输出文件 [stdout]\n"
"  -k file   从 RTKLIB 配置文件读入处理选项 [off]\n"
"  -e        输出XYZ-ECEF [LLH]\n"
"  -a        输出ENU基线 [LLH]\n"
"  -n        输出NMEA-0183 GGA语句 [off]\n"
"  -t        时间格式 yyyy/mm/dd hh:mm:ss [sssss.s]\n"
"  -u        输出UTC时间 [GPST]\n"
"  -s sep    字段分隔符 [' ']\n"
"  -d col    时间小数位数 [3]\n"
"  -y level  解统计级别 0关/1状态量/2残差 [0]\n"
"  -x level  调试跟踪级别 0关 [0]\n");
}

int main(int argc, char **argv)
{
    /* ○自动 使用 RTKLIB 内置默认值（rtkcmn.c 中 prcopt_default/solopt_default）*/
    prcopt_t prcopt=prcopt_default;
    solopt_t solopt=solopt_default;
    filopt_t filopt={""};
    gtime_t ts={0},te={0};                    /* ◇需设置 处理起止时间(0=不限) */
    double tint=0.0,                          /* ◇需设置 解算采样间隔(s,0=全取) */
           es[]={2000,1,1,0,0,0},ee[]={2000,12,31,23,59,59},pos[3];
    int i,j,n,ret;
    char *infile[MAXFILE],*outfile="",*p;

    /* ◇需设置 默认定位模式为动态RTK */
    prcopt.mode      =PMODE_KINEMA;           /* ◇需设置 定位模式(见 -p) */
    /* 默认值与 RTKLIB rnx2rtkp 一致；-sys 可显式选择其他星座 */
    prcopt.navsys    =SYS_GPS|SYS_GLO;
    prcopt.refpos    =POSOPT_RINEX;           /* ◇需设置 基准站坐标取RINEX观测文件头 */
    prcopt.glomodear =1;                      /* △可更改 GLONASS整周模糊度处理 */
    prcopt.bdsmodear =1;                      /* △可更改 北斗整周模糊度处理 */
    prcopt.modear    =ARMODE_CONT;            /* △可更改 整周模糊度: 1连续→2瞬时→3固定保持 */
    prcopt.ionoopt   =IONOOPT_BRDC;           /* △可更改 电离层: 广播模型；双频可用 -iono 3 */
    prcopt.tropopt   =TROPOPT_SAAS;           /* △可更改 对流层: Saastamoinen(长基线/静态可改3估计) */
    prcopt.soltype   =0;                      /* △可更改 解类型: 0=前向,1=后向,2=前后向结合 */
    prcopt.thresar[0]=3.0;                    /* △可更改 整周模糊度ratio检验阈值 */
    prcopt.maxtdiff  =30.0;                   /* △可更改 差分龄期上限(s) */
    prcopt.intpref   =1;                      /* △可更改 后处理时插值基准站观测 */
    solopt.timef     =0;                      /* △可更改 输出时间格式 */
    sprintf(solopt.prog,"%s ver.%s %s",PROGNAME,VER_RTKLIB,PATCH_LEVEL);
    sprintf(filopt.trace,"%s.trace",PROGNAME);

    /* 命令行参数解析 ---------------------------------------------------------*/
    /* 第一遍: 先处理 -k 配置文件（与 rnx2rtkp 一致: 配置先载入, 后续命令行选项覆盖）*/
    for (i=1;i<argc;i++) {
        if (!strcmp(argv[i],"-k")&&i+1<argc) {
            resetsysopts();
            if (!loadopts(argv[++i],sysopts)) {
                fprintf(stderr,"error : option load error: %s\n",argv[i]);
                return 1;
            }
            getsysopts(&prcopt,&solopt,&filopt);
        }
    }
    /* -k 会把 solopt.prog/filopt.trace 重置为空(rtkcmn.c solopt_default 无程序名)，
     * 这里重新写回程序名/默认跟踪文件名, 保证文件头标识正确(不改解算结果) */
    sprintf(solopt.prog,"%s ver.%s %s",PROGNAME,VER_RTKLIB,PATCH_LEVEL);
    if (!*filopt.trace) sprintf(filopt.trace,"%s.trace",PROGNAME);
    for (i=1,n=0;i<argc;i++) {
        if      (!strcmp(argv[i],"-o")&&i+1<argc) outfile=argv[++i];
        else if (!strcmp(argv[i],"-k")&&i+1<argc) {i++; continue;}
        else if (!strcmp(argv[i],"-ts")&&i+2<argc) {
            sscanf(argv[++i],"%lf/%lf/%lf",es,es+1,es+2);
            sscanf(argv[++i],"%lf:%lf:%lf",es+3,es+4,es+5);
            ts=epoch2time(es);                 /* ○自动 文本日期→gtime_t */
        }
        else if (!strcmp(argv[i],"-te")&&i+2<argc) {
            sscanf(argv[++i],"%lf/%lf/%lf",ee,ee+1,ee+2);
            sscanf(argv[++i],"%lf:%lf:%lf",ee+3,ee+4,ee+5);
            te=epoch2time(ee);
        }
        else if (!strcmp(argv[i],"-ti")&&i+1<argc) tint=atof(argv[++i]);
        else if (!strcmp(argv[i],"-p")&&i+1<argc) {
            int mode=atoi(argv[++i]);
            if (mode<PMODE_SINGLE||mode>PMODE_PPP_STATIC) {
                fprintf(stderr,"error : invalid positioning mode: %d\n",mode);
                return 2;
            }
            prcopt.mode=mode;
        }
        else if (!strcmp(argv[i],"-f")&&i+1<argc) {
            int nf=atoi(argv[++i]);
            if (nf<1||nf>3) {
                fprintf(stderr,"error : invalid frequency count: %d\n",nf);
                return 2;
            }
            prcopt.nf=nf;
        }
        else if (!strcmp(argv[i],"-sys")&&i+1<argc) {
            prcopt.navsys=0;
            for (p=argv[++i];*p;p++) {
                switch (*p) {
                    case 'G': prcopt.navsys|=SYS_GPS; break;
                    case 'R': prcopt.navsys|=SYS_GLO; break;
                    case 'E': prcopt.navsys|=SYS_GAL; break;
                    case 'J': prcopt.navsys|=SYS_QZS; break;
                    case 'C': prcopt.navsys|=SYS_CMP; break;
                    case 'I': prcopt.navsys|=SYS_IRN; break;
                    case ',': break;
                    default:
                        fprintf(stderr,"error : invalid navigation system: %c\n",*p);
                        return 2;
                }
            }
        }
        else if (!strcmp(argv[i],"-m")&&i+1<argc) prcopt.elmin=atof(argv[++i])*D2R;
        else if (!strcmp(argv[i],"-iono")&&i+1<argc) prcopt.ionoopt=atoi(argv[++i]);
        else if (!strcmp(argv[i],"-trop")&&i+1<argc) prcopt.tropopt=atoi(argv[++i]);
        else if (!strcmp(argv[i],"-v")&&i+1<argc) prcopt.thresar[0]=atof(argv[++i]);
        else if (!strcmp(argv[i],"-s")&&i+1<argc) {
            snprintf(solopt.sep,sizeof(solopt.sep),"%s",argv[++i]);
        }
        else if (!strcmp(argv[i],"-d")&&i+1<argc) solopt.timeu=atoi(argv[++i]);
        else if (!strcmp(argv[i],"-b")) prcopt.soltype=1;
        else if (!strcmp(argv[i],"-c")) prcopt.soltype=2;
        else if (!strcmp(argv[i],"-i")) prcopt.modear=ARMODE_INST;
        else if (!strcmp(argv[i],"-h")) prcopt.modear=ARMODE_FIXHOLD;
        else if (!strcmp(argv[i],"-t")) solopt.timef=1;
        else if (!strcmp(argv[i],"-u")) solopt.times=TIMES_UTC;
        else if (!strcmp(argv[i],"-e")) solopt.posf=SOLF_XYZ;
        else if (!strcmp(argv[i],"-a")) solopt.posf=SOLF_ENU;
        else if (!strcmp(argv[i],"-n")) solopt.posf=SOLF_NMEA;
        else if (!strcmp(argv[i],"-g")) solopt.degf=1;
        else if (!strcmp(argv[i],"-r")&&i+3<argc) {  /* ◇需设置 基准站坐标ECEF(当refpos=0时用) */
            prcopt.refpos=prcopt.rovpos=POSOPT_POS;
            for (j=0;j<3;j++) prcopt.rb[j]=atof(argv[++i]);
            matcpy(prcopt.ru,prcopt.rb,3,1);
        }
        else if (!strcmp(argv[i],"-l")&&i+3<argc) {  /* ◇需设置 基准站坐标经纬高 */
            prcopt.refpos=prcopt.rovpos=POSOPT_POS;
            for (j=0;j<3;j++) pos[j]=atof(argv[++i]);
            for (j=0;j<2;j++) pos[j]*=D2R;
            pos2ecef(pos,prcopt.rb);          /* ○自动 经纬高→ECEF */
            matcpy(prcopt.ru,prcopt.rb,3,1);  /* ○自动 流动站固定坐标=基准站坐标 */
        }
        else if (!strcmp(argv[i],"-y")&&i+1<argc) solopt.sstat=atoi(argv[++i]);
        else if (!strcmp(argv[i],"-x")&&i+1<argc) solopt.trace=atoi(argv[++i]);
        else if (!strcmp(argv[i],"-?")) { printhelp(); return 0; }  /* 显式帮助: 正常退出 */
        else if (*argv[i]=='-') { printhelp(); return 2; }          /* 未知选项: 帮助后报错 */
        else if (n<MAXFILE) infile[n++]=argv[i];   /* ◇需设置 输入文件 */
        else {
            fprintf(stderr,"error : too many input files (max=%d)\n",MAXFILE);
            return 2;
        }
    }
    if (!prcopt.navsys) prcopt.navsys=SYS_GPS|SYS_GLO;
    if (n<=0) {
        showmsg("error : no input file");
        printhelp();
        return 2;
    }
    for (i=0;i<n;i++) topath(infile[i]);   /* ○自动 正斜杠→反斜杠,防Windows路径截断 */
    if (*outfile) topath(outfile);
    /* ◇需设置 核心解算入口: 读入所有输入文件并逐历元解算,结果写 outfile */
    ret=postpos(ts,te,tint,0.0,&prcopt,&solopt,&filopt,infile,n,outfile,"","");

    if (!ret) fprintf(stderr,"%40s\r","");
    /* postpos(): 0=正常完成,-1=错误,1=用户中止；直接作为退出码返回。 */
    return ret;
}

