/*
 * s3c2410/s3c2440 的NAND Flash控制器接口
 * 修改自Linux内核2.6.13文件drivers/mtd/nand/s3c2410.c
 */

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_NAND) && !defined(CFG_NAND_LEGACY)
#include <s3c2410.h>
#include <nand.h>

DECLARE_GLOBAL_DATA_PTR;

#define S3C2410_NFSTAT_READY    (1<<0)
#define S3C2410_NFCONF_nFCE     (1<<11)

#define S3C2440_NFSTAT_READY    (1<<0)
#define S3C2440_NFCONT_nFCE     (1<<1)


/* S3C2410:NAND Flash的片选函数 */
static void s3c2410_nand_select_chip(struct mtd_info *mtd, int chip)
{
	S3C2410_NAND *const s3c2410nand = S3C2410_GetBase_NAND();

	if (chip == -1)
	{
		s3c2410nand->NFCONF |= S3C2410_NFCONF_nFCE; //禁止片选信号
	}
	else
	{
		s3c2410nand->NFCONF &= ~S3C2410_NFCONF_nFCE; //使能片选信号
	}
}


/* S3C2410:命令和控制函数
 *
 * 注意,这个函数仅仅根据各种命令来修改"写地址"IO_ADDR_W的值(这称为tglx方法),
 * 这种方法使得平台/开发板相关的代码很简单.
 * 真正发出命令是在上一层的NAND Flash的统一的驱动中实现,
 * 它首先调用这个函数修改"写地址",然后才分别发出控制,地址,数据序列.
 */
static void s3c2410_nand_hwcontrol(struct mtd_info *mtd, int cmd)
{
	S3C2410_NAND *const s3c2410nand = S3C2410_GetBase_NAND();
	struct nand_chip *chip = mtd->priv;

	switch (cmd)
	{
		case NAND_CTL_SETNCE:
		case NAND_CTL_CLRNCE:
			printf("%s: called for NCE\n", __FUNCTION__);
			break;

		case NAND_CTL_SETCLE:
			chip->IO_ADDR_W = (void *)&s3c2410nand->NFCMD;
			break;

		case NAND_CTL_SETALE:
			chip->IO_ADDR_W = (void *)&s3c2410nand->NFADDR;
			break;

			/* NAND_CTL_CLRCLE: */
			/* NAND_CTL_CLRALE: */

		default:
			chip->IO_ADDR_W =  (void *)&s3c2410nand->NFDATA;
			break;
	}
}


/* S3C2410:查询NAND Flash状态
 *
 * 返回值:0表示忙,1表示就绪
 */
static int s3c2410_nand_devready(struct mtd_info *mtd)
{
	S3C2410_NAND *const s3c2410nand = S3C2410_GetBase_NAND();

	return (s3c2410nand->NFSTAT & S3C2410_NFSTAT_READY);
}


/* S3C2440:NAND Flash的片选函数 */
static void s3c2440_nand_select_chip(struct mtd_info *mtd, int chip)
{
	S3C2440_NAND *const s3c2440nand = S3C2440_GetBase_NAND();

	if (chip == -1)
	{
		s3c2440nand->NFCONT |= S3C2440_NFCONT_nFCE; //禁止片选信号
	}
	else
	{
		s3c2440nand->NFCONT &= ~S3C2440_NFCONT_nFCE; //使能片选信号
	}
}


/* S3C2440:命令和控制函数
 *
 * 注意,这个函数仅仅根据各种命令来修改"写地址"IO_ADDR_W的值(这称为tglx方法),
 * 这种方法使得平台/开发板相关的代码很简单.
 * 真正发出命令是在上一层的NAND Flash的统一的驱动中实现,
 * 它首先调用这个函数修改"写地址",然后才分别发出控制,地址,数据序列.
 */
static void s3c2440_nand_hwcontrol(struct mtd_info *mtd, int cmd)
{
	S3C2440_NAND *const s3c2440nand = S3C2440_GetBase_NAND();
	struct nand_chip *chip = mtd->priv;

	switch (cmd)
	{
		case NAND_CTL_SETNCE:
		case NAND_CTL_CLRNCE:
			printf("%s: called for NCE\n", __FUNCTION__);
			break;

		case NAND_CTL_SETCLE:
			chip->IO_ADDR_W = (void *)&s3c2440nand->NFCMD;
			break;

		case NAND_CTL_SETALE:
			chip->IO_ADDR_W = (void *)&s3c2440nand->NFADDR;
			break;

			/* NAND_CTL_CLRCLE: */
			/* NAND_CTL_CLRALE: */

		default:
			chip->IO_ADDR_W =  (void *)&s3c2440nand->NFDATA;
			break;
	}
}


/* S3C2440:查询NAND Flash状态
 *
 * 返回值:0表示忙,1表示就绪
 */
static int s3c2440_nand_devready(struct mtd_info *mtd)
{
	S3C2440_NAND *const s3c2440nand = S3C2440_GetBase_NAND();

	return (s3c2440nand->NFSTAT & S3C2440_NFSTAT_READY);
}


/*
 * NAND Flash硬件初始化:
 * 设置NAND Flash的时序,使能NAND Flash控制器
 */
static void s3c24x0_nand_inithw(void)
{
	S3C2410_NAND *const s3c2410nand = S3C2410_GetBase_NAND();
	S3C2440_NAND *const s3c2440nand = S3C2440_GetBase_NAND();

#define TACLS       0
#define TWRPH0      4
#define TWRPH1      2

	if (gd->bd->bi_arch_number == MACH_TYPE_SMDK2410)
	{
		/* 使能NAND Flash控制器, 初始化ECC, 禁止片选, 设置时序 */
		s3c2410nand->NFCONF = (1 << 15) | (1 << 12) | (1 << 11) | (TACLS << 8) | (TWRPH0 << 4) | (TWRPH1 << 0);
	}
	else
	{
		/* 设置时序 */
		s3c2440nand->NFCONF = (TACLS << 12) | (TWRPH0 << 8) | (TWRPH1 << 4);
		/* 使能NAND Flash控制器, 初始化ECC, 禁止片选 */
		s3c2440nand->NFCONT = (1 << 4) | (0 << 1) | (1 << 0);
	}
}


/*
 * 被drivers/nand/nand.c 调用,初始化NAND Flash硬件,初始化访问接口函数
 */
void board_nand_init(struct nand_chip *chip)
{
	S3C2410_NAND *const s3c2410nand = S3C2410_GetBase_NAND();
	S3C2440_NAND *const s3c2440nand = S3C2440_GetBase_NAND();

	s3c24x0_nand_inithw(); //NAND Flash硬件初始化

	if (gd->bd->bi_arch_number == MACH_TYPE_SMDK2410)
	{
		chip->IO_ADDR_R = (void *)&s3c2410nand->NFDATA;
		chip->IO_ADDR_W =  (void *)&s3c2410nand->NFDATA;
		chip->hwcontrol = s3c2410_nand_hwcontrol;
		chip->dev_ready = s3c2410_nand_devready;
		chip->select_chip = s3c2410_nand_select_chip;
		chip->options = 0; //设置位宽等,位宽为8
	}
	else
	{
		chip->IO_ADDR_R = (void *)&s3c2440nand->NFDATA;
		chip->IO_ADDR_W =  (void *)&s3c2440nand->NFDATA;
		chip->hwcontrol = s3c2440_nand_hwcontrol;
		chip->dev_ready = s3c2440_nand_devready;
		chip->select_chip = s3c2440_nand_select_chip;
		chip->options = 0; //设置位宽等,位宽为8
	}
	chip->eccmode = NAND_ECC_SOFT; //ECC校验方式:软件ECC
}


#endif
