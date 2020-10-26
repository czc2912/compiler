/* 
 * PL/0 complier program (with syntax error recovery) implemented in C
 *
 * The program has been tested on Visual Studio 2010
 *
 * 使用方法：
 * 运行后输入PL/0源程序文件名
 * foutput.txt输出源文件及出错示意（如有错）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define bool int
#define true 1
#define false 0

#define norw 13   /* 保留字个数 */
#define txmax 100 /* 符号表容量 */
#define nmax 14   /* 数字的最大位数 */
#define al 10     /* 标识符的最大长度 */
#define maxerr 30 /* 允许的最多错误数 */

/* 符号 */
enum symbol
{
    nul,
    ident,
    number,
    plus,
    minus,
    times,
    slash,
    oddsym,
    eql,
    neq,
    lss,
    leq,
    gtr,
    geq,
    lparen,
    rparen,
    comma,
    semicolon,
    period,
    becomes,
    beginsym,
    endsym,
    ifsym,
    thensym,
    whilesym,
    writesym,
    readsym,
    dosym,
    callsym,
    constsym,
    varsym,
    procsym,
};
#define symnum 32

/* 符号表中的类型 */
enum object
{
    constant,
    variable,
    procedure,
};

char ch;                 /* 存放当前读取的字符，getch 使用 */
enum symbol sym;         /* 当前的符号 */
char id[al + 1];         /* 当前ident，多出的一个字节用于存放0 */
int num;                 /* 当前number */
int cc, ll;              /* getch使用的计数器，cc表示当前字符(ch)的位置 */
char line[81];           /* 读取行缓冲区 */
char a[al + 1];          /* 临时符号，多出的一个字节用于存放0 */
char word[norw][al];     /* 保留字 */
enum symbol wsym[norw];  /* 保留字对应的符号值 */
enum symbol ssym[256];   /* 单字符的符号值 */
bool declbegsys[symnum]; /* 表示声明开始的符号集合 */
bool statbegsys[symnum]; /* 表示语句开始的符号集合 */
bool facbegsys[symnum];  /* 表示因子开始的符号集合 */

/* 符号表结构 */
struct tablestruct
{
    char name[al];    /* 名字 */
    enum object kind; /* 类型：const，var或procedure */
};

struct tablestruct table[txmax]; /* 符号表 */

FILE *fin;     /* 输入源文件 */
FILE *foutput; /* 输出文件及出错示意（如有错） */
char fname[al];
int err; /* 错误计数器 */

void error(int n);
void getsym();
void getch();
void init();
void test(bool *s1, bool *s2, int n);
int inset(int e, bool *s);
int addset(bool *sr, bool *s1, bool *s2, int n);
int subset(bool *sr, bool *s1, bool *s2, int n);
int mulset(bool *sr, bool *s1, bool *s2, int n);
void block(int tx, bool *fsys);
void factor(bool *fsys, int *ptx);
void term(bool *fsys, int *ptx);
void condition(bool *fsys, int *ptx);
void expression(bool *fsys, int *ptx);
void statement(bool *fsys, int *ptx);
void vardeclaration(int *ptx);
void constdeclaration(int *ptx);
int position(char *idt, int tx);
void enter(enum object k, int *ptx);

/* 主程序开始 */
int main()
{
    bool nxtlev[symnum];

    printf("Input pl/0 file?   ");
    scanf("%s", fname); /* 输入文件名 */

    if ((fin = fopen(fname, "r")) == NULL)
    {
        printf("Can't open the input file!\n");
        exit(1);
    }

    ch = fgetc(fin);
    if (ch == EOF) /* 文件为空 */
    {
        printf("The input file is empty!\n");
        fclose(fin);
        exit(1);
    }
    rewind(fin);

    if ((foutput = fopen("foutput.txt", "w")) == NULL)
    {
        printf("Can't open the output file!\n");
        exit(1);
    }

    init(); /* 初始化 */
    err = 0;
    cc = ll = 0;
    ch = ' ';

    getsym();

    addset(nxtlev, declbegsys, statbegsys, symnum);
    nxtlev[period] = true;
    block(0, nxtlev); /* 处理分程序 */

    if (sym != period)
    {
        error(9);
    }

    if (err == 0)
    {
        printf("\n===Parsing success!===\n");
        fprintf(foutput, "\n===Parsing success!===\n");
    }
    else
    {
        printf("\n===%d errors in pl/0 program!===\n", err);
        fprintf(foutput, "\n===%d errors in pl/0 program!===\n", err);
    }

    fclose(foutput);
    fclose(fin);

    return 0;
}

/*
 * 初始化 
 */
void init()
{
    int i;

    /* 设置单字符符号 */
    for (i = 0; i <= 255; i++)
    {
        ssym[i] = nul;
    }
    ssym['+'] = plus;
    ssym['-'] = minus;
    ssym['*'] = times;
    ssym['/'] = slash;
    ssym['('] = lparen;
    ssym[')'] = rparen;
    ssym['='] = eql;
    ssym[','] = comma;
    ssym['.'] = period;
    ssym['#'] = neq;
    ssym[';'] = semicolon;

    /* 设置保留字名字,按照字母顺序，便于二分查找 */
    strcpy(&(word[0][0]), "begin");
    strcpy(&(word[1][0]), "call");
    strcpy(&(word[2][0]), "const");
    strcpy(&(word[3][0]), "do");
    strcpy(&(word[4][0]), "end");
    strcpy(&(word[5][0]), "if");
    strcpy(&(word[6][0]), "odd");
    strcpy(&(word[7][0]), "procedure");
    strcpy(&(word[8][0]), "read");
    strcpy(&(word[9][0]), "then");
    strcpy(&(word[10][0]), "var");
    strcpy(&(word[11][0]), "while");
    strcpy(&(word[12][0]), "write");

    /* 设置保留字符号 */
    wsym[0] = beginsym;
    wsym[1] = callsym;
    wsym[2] = constsym;
    wsym[3] = dosym;
    wsym[4] = endsym;
    wsym[5] = ifsym;
    wsym[6] = oddsym;
    wsym[7] = procsym;
    wsym[8] = readsym;
    wsym[9] = thensym;
    wsym[10] = varsym;
    wsym[11] = whilesym;
    wsym[12] = writesym;

    /* 设置符号集 */
    for (i = 0; i < symnum; i++)
    {
        declbegsys[i] = false;
        statbegsys[i] = false;
        facbegsys[i] = false;
    }

    /* 设置声明开始符号集 */
    declbegsys[constsym] = true;
    declbegsys[varsym] = true;
    declbegsys[procsym] = true;

    /* 设置语句开始符号集 */
    statbegsys[beginsym] = true;
    statbegsys[callsym] = true;
    statbegsys[ifsym] = true;
    statbegsys[whilesym] = true;

    /* 设置因子开始符号集 */
    facbegsys[ident] = true;
    facbegsys[number] = true;
    facbegsys[lparen] = true;
}

/*
 * 用数组实现集合的集合运算 
 */
int inset(int e, bool *s)
{
    return s[e];
}

int addset(bool *sr, bool *s1, bool *s2, int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        sr[i] = s1[i] || s2[i];
    }
    return 0;
}

int subset(bool *sr, bool *s1, bool *s2, int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        sr[i] = s1[i] && (!s2[i]);
    }
    return 0;
}

int mulset(bool *sr, bool *s1, bool *s2, int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        sr[i] = s1[i] && s2[i];
    }
    return 0;
}

/* 
 *	出错处理，打印出错位置和错误编码
 */
void error(int n)
{
    char space[81];
    memset(space, 32, 81);

    space[cc - 1] = 0; /* 出错时当前符号已经读完，所以cc-1 */

    printf("%s^%d\n", space, n);
    fprintf(foutput, "%s^%d\n", space, n);

    err = err + 1;
    if (err > maxerr)
    {
        exit(1);
    }
}

/*
 * 过滤空格，读取一个字符
 * 每次读一行，存入line缓冲区，line被getsym取空后再读一行
 * 被函数getsym调用
 */
void getch()
{
    if (cc == ll) /* 判断缓冲区中是否有字符，若无字符，则读入下一行字符到缓冲区中 */
    {
        if (feof(fin))
        {
            printf("Program incomplete!\n");
            exit(1);
        }
        ll = 0;
        cc = 0;

        ch = ' ';
        while (ch != 10)
        {
            if (EOF == fscanf(fin, "%c", &ch))
            {
                line[ll] = 0;
                break;
            }

            printf("%c", ch);
            fprintf(foutput, "%c", ch);
            line[ll] = ch;
            ll++;
        }
    }
    ch = line[cc];
    cc++;
}

/* 
 * 词法分析，获取一个符号
 */
void getsym()
{
    int i, j, k;

    while (ch == ' ' || ch == 10 || ch == 9) /* 过滤空格、换行和制表符 */
    {
        getch();
    }
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) /* 当前的单词是标识符或是保留字 */
    {
        k = 0;
        do
        {
            if (k < al)
            {
                a[k] = ch;
                k++;
            }
            getch();
        } while ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'));
        a[k] = 0;
        strcpy(id, a);
        i = 0;
        j = norw - 1;
        do
        { /* 搜索当前单词是否为保留字，使用二分法查找 */
            k = (i + j) / 2;
            if (strcmp(id, word[k]) <= 0)
            {
                j = k - 1;
            }
            if (strcmp(id, word[k]) >= 0)
            {
                i = k + 1;
            }
        } while (i <= j);
        if (i - 1 > j) /* 当前的单词是保留字 */
        {
            sym = wsym[k];
        }
        else /* 当前的单词是标识符 */
        {
            sym = ident;
        }
    }
    else
    {
        if (ch >= '0' && ch <= '9') /* 当前的单词是数字 */
        {
            k = 0;
            num = 0;
            sym = number;
            do
            {
                num = 10 * num + ch - '0';
                k++;
                getch();
            } while (ch >= '0' && ch <= '9'); /* 获取数字的值 */
            k--;
            if (k > nmax) /* 数字位数太多 */
            {
                error(30);
            }
        }
        else
        {
            if (ch == ':') /* 检测赋值符号 */
            {
                getch();
                if (ch == '=')
                {
                    sym = becomes;
                    getch();
                }
                else
                {
                    sym = nul; /* 不能识别的符号 */
                }
            }
            else
            {
                if (ch == '<') /* 检测小于或小于等于符号 */
                {
                    getch();
                    if (ch == '=')
                    {
                        sym = leq;
                        getch();
                    }
                    else
                    {
                        sym = lss;
                    }
                }
                else
                {
                    if (ch == '>') /* 检测大于或大于等于符号 */
                    {
                        getch();
                        if (ch == '=')
                        {
                            sym = geq;
                            getch();
                        }
                        else
                        {
                            sym = gtr;
                        }
                    }
                    else
                    {
                        sym = ssym[ch]; /* 当符号不满足上述条件时，全部按照单字符符号处理 */
                        if (sym != period)
                        {
                            getch();
                        }
                    }
                }
            }
        }
    }
}

/* 
 * 测试当前符号是否合法
 *
 * 在语法分析程序的入口和出口处调用测试函数test，
 * 检查当前单词进入和退出该语法单位的合法性
 *
 * s1:	需要的单词集合
 * s2:	如果不是需要的单词，在某一出错状态时，
 *      可恢复语法分析继续正常工作的补充单词符号集合
 * n:  	错误号
 */
void test(bool *s1, bool *s2, int n)
{
    if (!inset(sym, s1))
    {
        error(n);
        /* 当检测不通过时，不停获取符号，直到它属于需要的集合或补救的集合 */
        while ((!inset(sym, s1)) && (!inset(sym, s2)))
        {
            getsym();
        }
    }
}

/* 
 * 编译程序主体
 *
 * tx:     符号表当前尾指针
 * fsys:   当前模块后继符号集合
 */
void block(int tx, bool *fsys)
{
    int i;
    bool nxtlev[symnum]; /* 在下级函数的参数中，符号集合均为值参，但由于使用数组实现，
	                           传递进来的是指针，为防止下级函数改变上级函数的集合，开辟新的空间
	                           传递给下级函数*/
    do
    {

        if (sym == constsym) /* 遇到常量声明符号，开始处理常量声明 */
        {
            getsym();

            do
            {
                constdeclaration(&tx);
                while (sym == comma) /* 遇到逗号继续定义常量 */
                {
                    getsym();
                    constdeclaration(&tx);
                }
                if (sym == semicolon) /* 遇到分号结束定义常量 */
                {
                    getsym();
                }
                else
                {
                    error(5); /* 漏掉了分号 */
                }
            } while (sym == ident);
        }

        if (sym == varsym) /* 遇到变量声明符号，开始处理变量声明 */
        {
            getsym();

            do
            {
                vardeclaration(&tx);
                while (sym == comma)
                {
                    getsym();
                    vardeclaration(&tx);
                }
                if (sym == semicolon)
                {
                    getsym();
                }
                else
                {
                    error(5); /* 漏掉了分号 */
                }
            } while (sym == ident);
        }

        while (sym == procsym) /* 遇到过程声明符号，开始处理过程声明 */
        {
            getsym();

            if (sym == ident)
            {
                enter(procedure, &tx); /* 填写符号表 */
                getsym();
            }
            else
            {
                error(4); /* procedure后应为标识符 */
            }

            if (sym == semicolon)
            {
                getsym();
            }
            else
            {
                error(5); /* 漏掉了分号 */
            }

            memcpy(nxtlev, fsys, sizeof(bool) * symnum);
            nxtlev[semicolon] = true;
            block(tx, nxtlev); /* 递归调用 */

            if (sym == semicolon)
            {
                getsym();
                memcpy(nxtlev, statbegsys, sizeof(bool) * symnum);
                nxtlev[ident] = true;
                nxtlev[procsym] = true;
                test(nxtlev, fsys, 6);
            }
            else
            {
                error(5); /* 漏掉了分号 */
            }
        }
        memcpy(nxtlev, statbegsys, sizeof(bool) * symnum);
        nxtlev[ident] = true;
        test(nxtlev, declbegsys, 7);
    } while (inset(sym, declbegsys)); /* 直到没有声明符号 */

    /* 语句后继符号为分号或end */
    memcpy(nxtlev, fsys, sizeof(bool) * symnum); /* 每个后继符号集合都包含上层后继符号集合，以便补救 */
    nxtlev[semicolon] = true;
    nxtlev[endsym] = true;
    statement(nxtlev, &tx);
    memset(nxtlev, 0, sizeof(bool) * symnum); /* 分程序没有补救集合 */
    test(fsys, nxtlev, 8);                    /* 检测后继符号正确性 */
}

/* 
 * 在符号表中加入一项 
 *
 * k:      标识符的种类为const，var或procedure
 * ptx:    符号表尾指针的指针，为了可以改变符号表尾指针的值 
 * 
 */
void enter(enum object k, int *ptx)
{
    (*ptx)++;
    strcpy(table[(*ptx)].name, id); /* 符号表的name域记录标识符的名字 */
    table[(*ptx)].kind = k;
}

/* 
 * 查找标识符在符号表中的位置，从tx开始倒序查找标识符
 * 找到则返回在符号表中的位置，否则返回0
 * 
 * id:    要查找的名字
 * tx:     当前符号表尾指针
 */
int position(char *id, int tx)
{
    int i;
    strcpy(table[0].name, id);
    i = tx;
    while (strcmp(table[i].name, id) != 0)
    {
        i--;
    }
    return i;
}

/*
 * 常量声明处理 
 */
void constdeclaration(int *ptx)
{
    if (sym == ident)
    {
        getsym();
        if (sym == eql || sym == becomes)
        {
            if (sym == becomes)
            {
                error(1); /* 把=写成了:= */
            }
            getsym();
            if (sym == number)
            {
                enter(constant, ptx);
                getsym();
            }
            else
            {
                error(2); /* 常量声明中的=后应是数字 */
            }
        }
        else
        {
            error(3); /* 常量声明中的标识符后应是= */
        }
    }
    else
    {
        error(4); /* const后应是标识符 */
    }
}

/*
 * 变量声明处理 
 */
void vardeclaration(int *ptx)
{
    if (sym == ident)
    {
        enter(variable, ptx); // 填写符号表
        getsym();
    }
    else
    {
        error(4); /* var后面应是标识符 */
    }
}

/*
 * 语句处理 
 */
void statement(bool *fsys, int *ptx)
{
    int i;
    bool nxtlev[symnum];

    if (sym == ident) /* 准备按照赋值语句处理 */
    {
        i = position(id, *ptx); /* 查找标识符在符号表中的位置 */
        if (i == 0)
        {
            error(11); /* 标识符未声明 */
        }
        else
        {
            if (table[i].kind != variable)
            {
                error(12); /* 赋值语句中，赋值号左部标识符应该是变量 */
                i = 0;
            }
            else
            {
                getsym();
                if (sym == becomes)
                {
                    getsym();
                }
                else
                {
                    error(13); /* 没有检测到赋值符号 */
                }
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                expression(nxtlev, ptx); /* 处理赋值符号右侧表达式 */
            }
        }
    }
    else
    {
        if (sym == readsym) /* 准备按照read语句处理 */
        {
            getsym();
            if (sym != lparen)
            {
                error(34); /* 格式错误，应是左括号 */
            }
            else
            {
                do
                {
                    getsym();
                    if (sym == ident)
                    {
                        i = position(id, *ptx); /* 查找要读的变量 */
                    }
                    else
                    {
                        i = 0;
                    }

                    if (i == 0)
                    {
                        error(35); /* read语句括号中的标识符应该是声明过的变量 */
                    }
                    else
                    {
                    }
                    getsym();

                } while (sym == comma); /* 一条read语句可读多个变量 */
            }
            if (sym != rparen)
            {
                error(33);                /* 格式错误，应是右括号 */
                while (!inset(sym, fsys)) /* 出错补救，直到遇到上层函数的后继符号 */
                {
                    getsym();
                }
            }
            else
            {
                getsym();
            }
        }
        else
        {
            if (sym == writesym) /* 准备按照write语句处理 */
            {
                getsym();
                if (sym == lparen)
                {
                    do
                    {
                        getsym();
                        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                        nxtlev[rparen] = true;
                        nxtlev[comma] = true;
                        expression(nxtlev, ptx); /* 调用表达式处理 */
                    } while (sym == comma);      /* 一条write可输出多个变量的值 */
                    if (sym != rparen)
                    {
                        error(33); /* 格式错误，应是右括号 */
                    }
                    else
                    {
                        getsym();
                    }
                }
            }
            else
            {
                if (sym == callsym) /* 准备按照call语句处理 */
                {
                    getsym();
                    if (sym != ident)
                    {
                        error(14); /* call后应为标识符 */
                    }
                    else
                    {
                        i = position(id, *ptx);
                        if (i == 0)
                        {
                            error(11); /* 过程名未找到 */
                        }
                        else
                        {
                            if (table[i].kind != procedure)
                            {
                                error(15); /* call后标识符类型应为过程 */
                            }
                        }
                        getsym();
                    }
                }
                else
                {
                    if (sym == ifsym) /* 准备按照if语句处理 */
                    {
                        getsym();
                        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                        nxtlev[thensym] = true;
                        nxtlev[dosym] = true;   /* 后继符号为then或do */
                        condition(nxtlev, ptx); /* 调用条件处理（逻辑运算）函数 */
                        if (sym == thensym)
                        {
                            getsym();
                        }
                        else
                        {
                            error(16); /* 缺少then */
                        }
                        statement(fsys, ptx); /* 处理then后的语句 */
                    }
                    else
                    {
                        if (sym == beginsym) /* 准备按照复合语句处理 */
                        {
                            getsym();
                            memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                            nxtlev[semicolon] = true;
                            nxtlev[endsym] = true;  /* 后继符号为分号或end */
                            statement(nxtlev, ptx); /* 对begin与end之间的语句进行分析处理 */
                            /* 如果分析完一句后遇到语句开始符或分号，则循环分析下一句语句 */
                            while (inset(sym, statbegsys) || sym == semicolon)
                            {
                                if (sym == semicolon)
                                {
                                    getsym();
                                }
                                else
                                {
                                    error(10); /* 缺少分号 */
                                }
                                statement(nxtlev, ptx);
                            }
                            if (sym == endsym)
                            {
                                getsym();
                            }
                            else
                            {
                                error(17); /* 缺少end */
                            }
                        }
                        else
                        {
                            if (sym == whilesym) /* 准备按照while语句处理 */
                            {
                                getsym();
                                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                                nxtlev[dosym] = true;   /* 后继符号为do */
                                condition(nxtlev, ptx); /* 调用条件处理 */
                                if (sym == dosym)
                                {
                                    getsym();
                                }
                                else
                                {
                                    error(18); /* 缺少do */
                                }
                                statement(fsys, ptx); /* 循环体 */
                            }
                        }
                    }
                }
            }
        }
    }
    memset(nxtlev, 0, sizeof(bool) * symnum); /* 语句结束无补救集合 */
    test(fsys, nxtlev, 19);                   /* 检测语句结束的正确性 */
}

/*
 * 表达式处理 
 */
void expression(bool *fsys, int *ptx)
{
    bool nxtlev[symnum];

    if (sym == plus || sym == minus) /* 表达式开头有正负号，此时当前表达式被看作一个正的或负的项 */
    {
        getsym();
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(nxtlev, ptx); /* 处理项 */
    }
    else /* 此时表达式被看作项的加减 */
    {
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(nxtlev, ptx); /* 处理项 */
    }
    while (sym == plus || sym == minus)
    {

        getsym();
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(nxtlev, ptx); /* 处理项 */
    }
}

/*
 * 项处理 
 */
void term(bool *fsys, int *ptx)
{
    bool nxtlev[symnum];
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
    nxtlev[times] = true;
    nxtlev[slash] = true;
    factor(nxtlev, ptx); /* 处理因子 */
    while (sym == times || sym == slash)
    {
        getsym();
        factor(nxtlev, ptx);
    }
}

/* 
 * 因子处理 
 */
void factor(bool *fsys, int *ptx)
{
    int i;
    bool nxtlev[symnum];
    test(facbegsys, fsys, 24);    /* 检测因子的开始符号 */
    while (inset(sym, facbegsys)) /* 循环处理因子 */
    {

        if (sym == ident) /* 因子为常量或变量 */
        {
            i = position(id, *ptx); /* 查找标识符在符号表中的位置 */
            if (i == 0)
            {
                error(11); /* 标识符未声明 */
            }
            else
            {
                if (table[i].kind == procedure)
                {
                    error(21); /* 不能为过程 */
                }
            }
            getsym();
        }
        else
        {
            if (sym == number) /* 因子为数 */
            {
                getsym();
            }
            else
            {
                if (sym == lparen) /* 因子为表达式 */
                {
                    getsym();
                    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                    nxtlev[rparen] = true;
                    expression(nxtlev, ptx);
                    if (sym == rparen)
                    {
                        getsym();
                    }
                    else
                    {
                        error(22); /* 缺少右括号 */
                    }
                }
            }
        }
        memset(nxtlev, 0, sizeof(bool) * symnum);
        nxtlev[lparen] = true;
        test(fsys, nxtlev, 23); /* 一个因子处理完毕，遇到的单词应在fsys集合中 */
                                /* 如果不是，报错并找到下一个因子的开始，使语法分析可以继续运行下去 */
    }
}

/* 
 * 条件处理 
 */
void condition(bool *fsys, int *ptx)
{
    bool nxtlev[symnum];

    if (sym == oddsym) /* 准备按照odd运算处理 */
    {
        getsym();
        expression(fsys, ptx);
    }
    else
    {
        /* 逻辑表达式处理 */
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[eql] = true;
        nxtlev[neq] = true;
        nxtlev[lss] = true;
        nxtlev[leq] = true;
        nxtlev[gtr] = true;
        nxtlev[geq] = true;
        expression(nxtlev, ptx);
        if (sym != eql && sym != neq && sym != lss && sym != leq && sym != gtr && sym != geq)
        {
            error(20); /* 应该为关系运算符 */
        }
        else
        {
            getsym();
            expression(fsys, ptx);
        }
    }
}
