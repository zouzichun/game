
// game1.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEBUG 1

#define RND 250
#define CNT_LINE 22

#define ROUND_NUM    12

#define NEAR_RANGE 200

#define MARK_1_L 4
#define MARK_1_R 8
#define MARK_2_L 1
#define MARK_2_R 5

#define FULL_LINE 179

#define PATTERN_SIZE 100000


FILE *fp;
FILE *fp_cfg;
FILE *fp_data;
FILE *fp_match;
FILE *fp_out;
FILE *fp_result;
unsigned char data_pattern[PATTERN_SIZE];

typedef struct
{
	char val_range_1;
	char val_range_2;
}  DIS_BUF;

struct gl_info_t
{
	int count;
	int index;
	char ex_flag;
	char data_val[RND][10];
	int  val_store[RND];
	DIS_BUF buf[RND];
} gl_info;

typedef enum
{
	IN_1_5,
	IN_6_10,
	IN_4_8,
	IN_123_90,
} RES_DESC;

typedef struct
{
	int display_flag[RND];
	int cnt;
	int row[RND];
	int colum[RND];
	RES_DESC next_pre;
	int  val_offset;
}  RES_AB_INFO;

typedef struct
{
	int display_flag[RND];
	int cnt;
	int row[RND];
	int colum[RND];
	RES_DESC next_pre;
	int  val_offset;
}  RES_AAB_INFO;

typedef struct
{
	int display_flag[RND];
	int cnt;
	int row[RND];
	int colum[RND];
	RES_DESC next_pre;
	int  val_offset;
}  RES_AABB_INFO;

typedef struct
{
	int total;// 数据库总个数设置
	int up_limit;//查找的循环上线
	int lw_limit;//查找的循环下线
	int cnt_limit;//保留的结果上线
}  CFG_INFO;


CFG_INFO cfg_info;
char tmp_buffer[RND][10];

char temp_str[200];    // 临时子串
char pi[100][FULL_LINE];
char gold[100][FULL_LINE];
char pi2[1020][FULL_LINE];
char new_line[2] = { 0x0d,0x0a };
char blank = { 0x9 };


typedef enum
{
	BASIC = 1,
	BASIC_N,
	BASIC_P,
	PLUS,
	N1_P_N,
	N1_P_N_R,
	C_MUL1,
	C_MUL2,
	PI,
	GOLD,
	PI2,
}  SHEET_TYPE;

typedef struct
{
	int num;
	SHEET_TYPE type;
	char *name;
	int rounds;
	int pages;
	int cols;
}TEST;

TEST tests[30] =
{
	{ 0,BASIC,"横向基本",ROUND_NUM,1,13 },
	{ 1,PLUS,"加",ROUND_NUM,9,10 },
	{ 2,BASIC_N,"11-1",ROUND_NUM,1,13 },
	{ 3,BASIC_P,"12+1",ROUND_NUM,1,13 },
	{ 4,N1_P_N,"13纵",ROUND_NUM,1,10 },
	{ 5,N1_P_N_R,"14右",ROUND_NUM,1,9 },
	{ 6,C_MUL1,"C-1",ROUND_NUM,1,9 },
	{ 7,C_MUL2,"C-2",ROUND_NUM,1,8 },
	{ 8,PI,"PI",ROUND_NUM,8,12 },
	{ 9,GOLD,"gold",ROUND_NUM,1,11 },
	//{10,PI2,"new-PI",ROUND_NUM,100,10},
};

int mask_table[30][3] = { { 2,0,5 },{ 4,0,7 },{ 6,0,9 },{ 3,0,8 },{ 1,0,6 },
{ 1,3,5 },{ 2,4,6 },{ 1,5,9 },{ 2,3,5 },{ 5,0,6 },
{ 1,0,7 },{ 2,5,8 },{ 1,0,2 } };

char string_pre[4][30] = {
	"出现在1-5",
	"出现在6-10",
	"出现在4-8",
	"出现在1,2,3,9,10",
};

int GetSubStrPos(char *str1, char *str2);

void ReadStrUnit(char * str, char *temp_str, int idx, int len)  // 从母串中获取与子串长度相等的临时子串

{
	int index = 0;
	for (index; index < len; index++)
	{
		temp_str[index] = str[idx + index];
	}
	temp_str[index] = '\0';
}

int GetSubStrPos(char *str1, char *str2)
{
	int idx = 0;
	int len1 = strlen(str1);
	int len2 = strlen(str2);

	if (len1 < len2)
	{
		//printf("err 1 \n"); // 子串比母串长
		return -1;
	}

	while (1)
	{
		ReadStrUnit(str1, temp_str, idx, len2);    // 不断获取的从 母串的 idx 位置处更新临时子串

		if (strcmp(str2, temp_str) == 0)break;      // 若临时子串和子串一致，结束循环

		idx++;                                  // 改变从母串中取临时子串的位置
		if (idx >= len1)return -1;                 // 若 idx 已经超出母串长度，说明母串不包含该子串

	}

	return idx;    // 返回子串第一个字符在母串中的位置
}

unsigned int rnd_op(unsigned int i)
{
	return (i%RND);
}

unsigned int rnd_over(unsigned int i,unsigned int top)
{
	return (i%top);
}

int check_valid(char *p)
{
	int i;
	char tmp;
	for (i = 0; i<CNT_LINE; i++)
	{
		tmp = *p;
		// 0-9 Tab Enter
		if (((tmp >= '0') && (tmp <= '9')) || (tmp == 0x9) || (tmp == 0x0d) || (tmp == 0x0a))

		{
			p++;
		}
		else
		{
			return 0;
		}
	}
	return 1;
}

int count_line(FILE *f)
{
	int line = 0;
	char c;
	while ((c = fgetc(f)) != EOF) if (c == '\n') line++;
	rewind(f);
	return line;
}

int process_data_val(int file_nums)
{
	int lines,i,j,pos;
	char c,c1;
	char file_name[20], tmp[4];

	FILE *fp_data;
	pos = 0;
	for (j = 1; j <= file_nums; j++)
	{
		file_name[0] = 0;// zero string
		itoa(j, tmp, 10);
		strcat(file_name, "data");
		strcat(file_name, tmp);
		strcat(file_name, ".txt");

		fp_data = fopen(file_name, "rb");
		if (fp_data == NULL){
			printf("no data faile with name %s!\n", file_name);
			getchar();
			return 0;
		}
		
		lines = count_line(fp_data);
		for (i = 0; i < lines; i++)
		{
			do {
				c = fgetc(fp_data);
				if (c >= '0' && c <= '9')
				{
					data_pattern[pos] = c - '0';
					if (c == '0')
						data_pattern[pos] = 10;
					pos++;
				}
			} while (c != '\n');
		}
		fclose(fp_data);
	}
}

// get data value from file
int get_data_val(FILE *fp)
{
	char tmp[CNT_LINE];
	int i, pos;
	// 22 bytes every line
	while (fread(tmp, 1, CNT_LINE, fp) != 0)
	{
		if (check_valid(tmp) == 0)
		{
			printf("文本数据有误\n");
			getchar();
			return 0;
		}
		pos = 0;
		for (i = 0; i < CNT_LINE; i++)
		{
			if (tmp[i] == 0xd)
				break;

			if (tmp[i] == 0x9)
				continue;
			else if (tmp[i] == '1' && tmp[i + 1] == '0')
			{
				gl_info.data_val[rnd_op(gl_info.index)][pos] = 10;
				i++;
				pos++;
			}
			else
			{
				gl_info.data_val[rnd_op(gl_info.index)][pos] = tmp[i] - '0';
				pos++;
			}
		}
		gl_info.index++;
	}

	if (gl_info.index > RND)
		gl_info.ex_flag = 1;

	return gl_info.index;
}


void up_side_down(int cnt)
{
	int i, j;
	char t_buff[10];
	for (i = 0; i<cnt / 2; i++)
	{
		for (j = 0; j<10; j++)
			t_buff[j] = tmp_buffer[i][j];
		for (j = 0; j<10; j++)
			tmp_buffer[i][j] = tmp_buffer[cnt - 1 - i][j];
		for (j = 0; j<10; j++)
			tmp_buffer[cnt - 1 - i][j] = t_buff[j];
	}
}


// copy data from gl_info.data_val to tmp_buffer
void round_buffer(void)
{
	int pos, cnt, i, j;

	cnt = 0;
	if (gl_info.index > RND)
	{
		pos = (gl_info.index) % RND;

		for (i = pos; i<RND; i++)
		{
			for (j = 0; j<10; j++)
				tmp_buffer[cnt][j] = gl_info.data_val[i][j];
			cnt++;
		}
		for (i = 0; i<pos; i++)
		{
			for (j = 0; j<10; j++)
				tmp_buffer[cnt][j] = gl_info.data_val[i][j];
			cnt++;
		}
	}
	else
	{
		for (i = 0; i<gl_info.index; i++)
		{
			for (j = 0; j<10; j++)
				tmp_buffer[cnt][j] = gl_info.data_val[i][j];
			cnt++;
		}
	}

}

int build_col(FILE *in, FILE *out, char(*buff)[FULL_LINE])
{

	char tmp[FULL_LINE];
	int  stat[11] = { 0 };
	int cnt = 0;
	int i, pos = 0;

	while (fread(tmp, 1, FULL_LINE, in) != 0)
	{
		for (i = 0; i<FULL_LINE; i++)
		{
			fwrite(&tmp[i], 1, 1, fp_out);
			fwrite(&blank, 1, 1, fp_out);
			if (tmp[i] == '0')
				tmp[i] = 10;
			else
				tmp[i] = tmp[i] - '0';
			cnt++;
			stat[tmp[i]]++;
		}
		fwrite(new_line, 1, 2, fp_out);
		memcpy(buff++, tmp, FULL_LINE);
	}
	fprintf(fp_out, "统计数据\n");
	fprintf(fp_out, "总数%d\n", cnt);
	for (i = 1; i<11; i++)
		fprintf(fp_out, "%3d: 总数%6d  比例 %f \n", i, stat[i], ((float)stat[i] / cnt) * 100);
	return pos;

}
int build_col_with_output(FILE *in, FILE *out, char(*buff)[FULL_LINE])
{
	char tmp[FULL_LINE];
	int  stat[11] = { 0 };
	int cnt = 0;
	int i, pos = 0;

	while (fread(tmp, 1, FULL_LINE, in) != 0)
	{
		for (i = 0; i<FULL_LINE; i++)
		{
			if (tmp[i] == '0')
				tmp[i] = 10;
			else
				tmp[i] = tmp[i] - '0';
		}
		memcpy(buff++, tmp, FULL_LINE);
	}

	return pos;
}

int range_cal(SHEET_TYPE type, int plus_val, int add_val, int left, int mid, int right, int col)
{
	int val, i, index;
	int cnt;

	if (gl_info.index > RND)
		cnt = RND;
	else
		cnt = gl_info.index;

	for (index = 0; index<cnt; index++)
	{
		gl_info.buf[index].val_range_1 = 'A';
		gl_info.buf[index].val_range_2 = 'A';
	}

	for (index = 0; index<RND; index++)
	{
		if (index == 0 && (type != PI) && (type != GOLD))
			val = 10;
		else if ((index == 1) && ((type == N1_P_N) || (type == N1_P_N_R)))
			val = 10;
		else
		{
			switch (type)
			{
			case BASIC:
				if (mid != 0)
					val = tmp_buffer[index - 1][left - 1] + tmp_buffer[index - 1][right - 1] + tmp_buffer[index - 1][mid - 1];
				else
					val = tmp_buffer[index - 1][left - 1] + tmp_buffer[index - 1][right - 1];
				break;
			case BASIC_N:
				if (mid != 0)
					val = tmp_buffer[index - 1][left - 1] + tmp_buffer[index - 1][right - 1] + tmp_buffer[index - 1][mid - 1] - 1;
				else
					val = tmp_buffer[index - 1][left - 1] + tmp_buffer[index - 1][right - 1] - 1;
				break;
			case BASIC_P:
				if (mid != 0)
					val = tmp_buffer[index - 1][left - 1] + tmp_buffer[index - 1][right - 1] + tmp_buffer[index - 1][mid - 1] + 1;
				else
					val = tmp_buffer[index - 1][left - 1] + tmp_buffer[index - 1][right - 1] + 1;
				break;
			case PLUS:
				val = tmp_buffer[index - 1][plus_val] + add_val;
				break;

			case N1_P_N:
				val = tmp_buffer[index - 2][plus_val] + tmp_buffer[index - 1][plus_val];
				break;

			case N1_P_N_R:
				val = tmp_buffer[index - 2][plus_val] + tmp_buffer[index - 1][plus_val + 1];
				break;

			case C_MUL1:
				val = tmp_buffer[index - 1][0] * tmp_buffer[index - 1][plus_val + 1];
				break;
			case C_MUL2:
				val = tmp_buffer[index - 1][1] * tmp_buffer[index - 1][plus_val + 2];
				break;
			case PI:
				val = pi[col][index];
				break;
			case GOLD:
				val = gold[col][index];
				break;
			case PI2:
				val = pi2[col][index];
				break;
				//case default;
				//break;
			}
		}

		while (val > 10)
			val = val % 10;

		// in case val == 30
		if (val == 0)
			val = 10;

		gl_info.val_store[index] = val;

		for (i = 0; i<10; i++)
		{
			if (tmp_buffer[index][i] == val)
			{
				if ((i >= (MARK_1_L - 1)) && (i <= (MARK_1_R - 1)))
					gl_info.buf[index].val_range_1 = 'B';
				if ((i >= (MARK_2_L - 1)) && (i <= (MARK_2_R - 1)))
					gl_info.buf[index].val_range_2 = 'B';
				break;
			}
		}
	}
	return 0;
}

// this function used for mark ABABAB
int range_cal_pattern(int offset)
{
	int val, i, index;
	int cnt;

	cnt = gl_info.index;

	for (index = 0; index<cnt; index++)
	{
		gl_info.buf[index].val_range_1 = 'A';
		gl_info.buf[index].val_range_2 = 'A';
	}

	for (index = 0; index<RND; index++)
	{
		val = data_pattern[offset + index];

		if (val == 0)
			val = 10;

		gl_info.val_store[index] = val;
	}

	for (index = 0; index<cnt; index++)
	{
		val = gl_info.val_store[index];
		for (i = 0; i<10; i++)
		{
			if (tmp_buffer[index][i] == val)
			{
				if ((i >= (MARK_1_L - 1)) && (i <= (MARK_1_R - 1)))
					gl_info.buf[index].val_range_1 = 'B';
				if ((i >= (MARK_2_L - 1)) && (i <= (MARK_2_R - 1)))
					gl_info.buf[index].val_range_2 = 'B';
				break;
			}
		}
	}
	return 0;
}

RES_AB_INFO t_ab;
RES_AAB_INFO t_aab;
RES_AABB_INFO t_aabb;


int rule_AB(int val, int col)
{
	int index, cnt, rnd, i;
	int pos, pos_tmp = 0;;
	char tmp[RND + 1], mask_buf[RND + 1], *p;

	rnd = val;
	p = tmp;
	if (gl_info.index > RND)
		cnt = RND;
	else
		cnt = gl_info.index;

	while (rnd--)
	{
		*p++ = 'A';
		*p++ = 'B';
	}
	*p++ = 0;

	for (i = 0; i<2; i++)
	{
		p = mask_buf;
		pos_tmp = 0;
		if (i == 0)
			for (index = cnt - 1; index >= 0; index--)
			{
				*p++ = gl_info.buf[index].val_range_1;
			}
		else
			for (index = cnt - 1; index >= 0; index--)
			{
				*p++ = gl_info.buf[index].val_range_2;
			}
		*p = 0;

		do
		{
			pos = GetSubStrPos(mask_buf + pos_tmp, tmp);

			if (pos != -1)
			{
				if (pos_tmp == 0)
				{
					if (pos == 0)
					{
						t_ab.display_flag[t_ab.cnt] = 1;
						t_ab.val_offset = 0;
						if (i == 0)
						{
							t_ab.next_pre = IN_123_90;
						}
						else
						{
							t_ab.next_pre = IN_6_10;
						}
					}
					if (pos == 1 && mask_buf[0] == 'B')
					{
						t_ab.display_flag[t_ab.cnt] = 1;
						t_ab.val_offset = 1;
						if (i == 0)
						{
							t_ab.next_pre = IN_4_8;
						}
						else
						{
							t_ab.next_pre = IN_1_5;
						}
					}
				}
				pos_tmp += pos_tmp + pos + val * 2;
				pos = cnt - (pos_tmp - val * 2);

				if (gl_info.ex_flag)
					pos = ((gl_info.index / RND) - 1)*RND + gl_info.index%RND + pos + 1;
				t_ab.row[t_ab.cnt] = pos;
				t_ab.colum[t_ab.cnt] = col * 2 + 1 + i;
				t_ab.cnt++;
				//printf("模型AB，循环%3d次，出现在%3d行，%3d列\n",val,pos,i+col+1);
			}
		} while (pos != -1);
	}

	return 1;
}
int rule_AAB(int val, int col)
{
	int index, cnt, rnd, i;
	int pos, pos_tmp = 0;;
	char tmp[RND + 1], mask_buf[RND + 1], *p;

	rnd = val;
	p = tmp;
	if (gl_info.index > RND)
		cnt = RND;
	else
		cnt = gl_info.index;

	while (rnd--)
	{
		*p++ = 'A';
		*p++ = 'A';
		*p++ = 'B';
	}
	*p++ = 0;

	for (i = 0; i<2; i++)
	{
		p = mask_buf;
		pos_tmp = 0;
		if (i == 0)
			for (index = cnt - 1; index >= 0; index--)
			{
				*p++ = gl_info.buf[index].val_range_1;
			}
		else
			for (index = cnt - 1; index >= 0; index--)
			{
				*p++ = gl_info.buf[index].val_range_2;
			}
		*p = 0;

		do
		{
			pos = GetSubStrPos(mask_buf + pos_tmp, tmp);

			if (pos != -1)
			{
				if (pos_tmp == 0)
				{
					switch (pos)
					{
					case 0:
						t_aab.display_flag[t_aab.cnt] = 1;
						t_aab.val_offset = 0;
						if (i == 0)
						{
							t_aab.next_pre = IN_123_90;
						}
						else
						{
							t_aab.next_pre = IN_6_10;
						}
						break;
					case 1:
						if (mask_buf[0] == 'B')
						{
							t_aab.display_flag[t_aab.cnt] = 1;
							t_aab.val_offset = 1;
							if (i == 0)
							{
								t_aab.next_pre = IN_4_8;
							}
							else
							{
								t_aab.next_pre = IN_1_5;
							}
						}
						break;
					case 2:
						if (mask_buf[0] == 'A' && mask_buf[1] == 'B')
						{
							t_aab.display_flag[t_aab.cnt] = 1;
							t_aab.val_offset = 2;
							if (i == 0)
							{
								t_aab.next_pre = IN_4_8;
							}
							else
							{
								t_aab.next_pre = IN_1_5;
							}
						}
						break;
					default:
						break;
					}
				}
				pos_tmp += pos_tmp + pos + val * 3;
				pos = cnt - (pos_tmp - val * 3);

				if (gl_info.ex_flag)
					pos = ((gl_info.index / RND) - 1)*RND + gl_info.index%RND + pos + 1;
				t_aab.row[t_aab.cnt] = pos;
				t_aab.colum[t_aab.cnt] = col * 2 + 1 + i;
				t_aab.cnt++;
				//printf("模型AB，循环%3d次，出现在%3d行，%3d列\n",val,pos,i+col+1);
			}
		} while (pos != -1);
	}

	return 1;
}
int rule_AABB(int val, int col)
{
	int index, cnt, rnd, i;
	int pos, pos_tmp = 0;
	char tmp[RND + 1], mask_buf[RND + 1], *p;

	rnd = val;
	p = tmp;
	if (gl_info.index > RND)
		cnt = RND;
	else
		cnt = gl_info.index;

	while (rnd--)
	{
		*p++ = 'A';
		*p++ = 'A';
		*p++ = 'B';
		*p++ = 'B';
	}
	*p++ = 0;

	for (i = 0; i<2; i++)
	{
		p = mask_buf;
		pos_tmp = 0;
		if (i == 0)
			for (index = cnt - 1; index >= 0; index--)
			{
				*p++ = gl_info.buf[index].val_range_1;
			}
		else
			for (index = cnt - 1; index >= 0; index--)
			{
				*p++ = gl_info.buf[index].val_range_2;
			}
		*p = 0;

		do
		{
			pos = GetSubStrPos(mask_buf + pos_tmp, tmp);

			if (pos != -1)
			{
				if (pos_tmp == 0)
				{
					switch (pos)
					{
					case 0:
						t_aabb.display_flag[t_aabb.cnt] = 1;
						t_aabb.val_offset = 0;
						if (i == 0)
						{
							t_aabb.next_pre = IN_123_90;
						}
						else
						{
							t_aabb.next_pre = IN_6_10;
						}
						break;
					case 1:
						if (mask_buf[0] == 'B')
						{
							t_aabb.display_flag[t_aabb.cnt] = 1;
							t_aabb.val_offset = 1;
							if (i == 0)
							{
								t_aabb.next_pre = IN_123_90;
							}
							else
							{
								t_aabb.next_pre = IN_6_10;
							}
						}
						break;
					case 2:
						if (mask_buf[0] == 'B' && mask_buf[1] == 'B')
						{
							t_aabb.display_flag[t_aabb.cnt] = 1;
							t_aabb.val_offset = 2;
							if (i == 0)
							{
								t_aabb.next_pre = IN_4_8;
							}
							else
							{
								t_aabb.next_pre = IN_1_5;
							}
						}
						break;
					case 3:
						if (mask_buf[0] == 'A' && mask_buf[1] == 'B' && mask_buf[2] == 'B')
						{
							t_aabb.display_flag[t_aabb.cnt] = 1;
							t_aabb.val_offset = 3;
							if (i == 0)
							{
								t_aabb.next_pre = IN_4_8;
							}
							else
							{
								t_aabb.next_pre = IN_1_5;
							}
						}
						break;
					default:
						break;
					}
				}
				pos_tmp += pos_tmp + pos + val * 4;
				pos = cnt - (pos_tmp - val * 4);

				if (gl_info.ex_flag)
					pos = ((gl_info.index / RND) - 1)*RND + gl_info.index%RND + pos + 1;
				t_aabb.row[t_aabb.cnt] = pos;
				t_aabb.colum[t_aabb.cnt] = col * 2 + 1 + i;
				t_aabb.cnt++;
				//printf("模型AB，循环%3d次，出现在%3d行，%3d列\n",val,pos,i+col+1);
			}
		} while (pos != -1);
	}

	return 1;
}


void range_large2small(RES_AB_INFO *p)
{
	int i, j;
	int cnt = p->cnt;
	int temp;

	for (j = 0; j<cnt; j++)
	{
		for (i = 0; i<cnt - j; i++)
			if (p->row[i] < p->row[i + 1])
			{
				temp = p->row[i];
				p->row[i] = p->row[i + 1];
				p->row[i + 1] = temp;

				temp = p->colum[i];
				p->colum[i] = p->colum[i + 1];
				p->colum[i + 1] = temp;

				temp = p->display_flag[i];
				p->display_flag[i] = p->display_flag[i + 1];
				p->display_flag[i + 1] = temp;
			}
	}
}

void display_easy(int rnd_num, int near_range)
{
	int j;
	int flag = 0;

	for (j = 0; j<t_ab.cnt; j++)
	{
		if ((t_ab.row[j] != 0) && t_ab.display_flag[j])
		{
			if (flag == 0)
			{
				//printf(fp_out,"--AB--循环 %2d次：-------\n",rnd_num);
				//printf(fp_out,"  行     列   \n");
				flag = 1;
			}
			printf("AB	%2d次	%3d列   \n", rnd_num, t_ab.colum[j]);
			printf("%d 预测%s\n", gl_info.val_store[t_ab.row[j] + t_ab.val_offset], string_pre[t_ab.next_pre]);
		}
	}

	flag = 0;
	for (j = 0; j<t_aab.cnt; j++)
	{
		if ((t_aab.row[j] != 0) && t_aab.display_flag[j])
		{
			if (flag == 0)
			{
				//fprintf(fp_out,"--AAB--循环 %2d次：-------\n",rnd_num-1);
				//fprintf(fp_out,"  行     列   \n");
				flag = 1;
			}
			printf("AAB	%2d次 %3d列   \n", rnd_num - 1, t_aab.colum[j]);
			printf("%d 预测%s\n", gl_info.val_store[t_aab.row[j] + t_aab.val_offset], string_pre[t_aab.next_pre]);
		}
	}

	flag = 0;
	for (j = 0; j<t_aabb.cnt; j++)
	{
		if ((t_aabb.row[j] != 0) && t_aabb.display_flag[j])
		{
			if (flag == 0)
			{
				//fprintf(fp_out,"--AABB--循环 %2d次：-------\n",rnd_num-2);
				//fprintf(fp_out,"  行     列   \n");
				flag = 1;
			}
			printf("AABB	%2d次 %3d列   \n", rnd_num - 2, t_aabb.colum[j]);
			printf("%d 预测%s\n", gl_info.val_store[t_aabb.row[j] + t_aabb.val_offset], string_pre[t_aabb.next_pre]);
		}
	}
}

void display_result(int rnd_num, int near_range)
{
	int j;
	int flag = 0;

	for (j = 0; j<t_ab.cnt; j++)
	{
		if ((t_ab.row[j] != 0) && ((gl_info.index - t_ab.row[j]) < near_range))
		{
			if (flag == 0)
			{
				fprintf(fp_result, "--AB--循环 %2d次：-------\n", rnd_num);
				fprintf(fp_result, "  行     列   \n");
				flag = 1;
			}
			fprintf(fp_result, " %3d   %3d   \n", t_ab.row[j], t_ab.colum[j]);
		}
	}

	flag = 0;
	for (j = 0; j<t_aab.cnt; j++)
	{
		if ((t_aab.row[j] != 0) && ((gl_info.index - t_aab.row[j]) < near_range))
		{
			if (flag == 0)
			{
				fprintf(fp_result, "--AAB--循环 %2d次：-------\n", rnd_num - 1);
				fprintf(fp_result, "  行     列   \n");
				flag = 1;
			}
			fprintf(fp_result, " %3d   %3d   \n", t_aab.row[j], t_aab.colum[j]);
		}
	}

	flag = 0;
	for (j = 0; j<t_aabb.cnt; j++)
	{
		if ((t_aabb.row[j] != 0) && ((gl_info.index - t_aabb.row[j]) < near_range))
		{
			if (flag == 0)
			{
				fprintf(fp_result, "--AABB--循环 %2d次：-------\n", rnd_num - 2);
				fprintf(fp_result, "  行     列   \n");
				flag = 1;
			}
			fprintf(fp_result, " %3d   %3d   \n", t_aabb.row[j], t_aabb.colum[j]);
		}
	}
}


void ts_run(TEST *p)
{
	int rnd_num, i, k;

	for (k = 1; k <= p->pages; k++)
	{
		printf("-------------%s% 3d----------------------\n", p->name, k);
		for (rnd_num = p->rounds; rnd_num>3; rnd_num--)
		{
			for (i = 0; i< p->cols; i++)
			{
				memset(&t_ab, 0, sizeof(RES_AB_INFO));
				memset(&t_aab, 0, sizeof(RES_AAB_INFO));
				memset(&t_aabb, 0, sizeof(RES_AABB_INFO));

				range_cal(p->type, i, k, mask_table[i][0], mask_table[i][1], mask_table[i][2], (k - 1) * 12 + i);
				rule_AB(rnd_num, i);
				rule_AAB(rnd_num - 1, i);
				rule_AABB(rnd_num - 2, i);

				range_large2small((RES_AB_INFO *)(&t_ab));
				range_large2small((RES_AB_INFO *)(&t_aab));
				range_large2small((RES_AB_INFO *)(&t_aabb));

				fprintf(fp_result, "-------------%s %3d------------%2d-----------\n", p->name, k, rnd_num);
				display_result(rnd_num, NEAR_RANGE);
				display_easy(rnd_num, NEAR_RANGE);

			}

		}
	}
}

int find_run(void)
{
	int rnd_num, i, j, display_cnt = 0;
	unsigned int position, tmp_pos;

	char tmp;
	char new_line[2] = { 0x0d,0x0a };
	char num_10[2] = { 0x31,0x30 };
	char tab = 0x9;

	//srand((int)time(NULL));     //时间产生种子
	//position = rnd_over(rand(), cfg_info.total - RND);
	//position = 0;

	for (i = 0; i<cfg_info.total - RND; i++)
	{
		//tmp_pos = rnd_over(position + i, cfg_info.total - RND);
		tmp_pos = i;
		for (rnd_num = cfg_info.up_limit; rnd_num>=cfg_info.lw_limit; rnd_num--)
		{

			range_cal_pattern(tmp_pos);

			memset(&t_ab, 0, sizeof(RES_AB_INFO));
			rule_AB(rnd_num, 0);

			range_large2small((RES_AB_INFO *)(&t_ab));

			if (t_ab.display_flag[0])
			{
				display_cnt++;
				if (display_cnt <= cfg_info.cnt_limit)
				{
					printf("%d 位置找到匹配，循环%2d次 \n", tmp_pos, rnd_num);
					printf("%d 预测%s\n", gl_info.val_store[t_ab.row[0] + t_ab.val_offset], string_pre[t_ab.next_pre]);
				}
				else if (display_cnt == (cfg_info.cnt_limit + 1))
				{
					printf("到达最大显示上限 %d，数据将被记录到文件 \n",cfg_info.cnt_limit);
				}
				
				fprintf(fp_match,"%d 位置找到匹配，循环%2d次 \n", tmp_pos, rnd_num);
				fprintf(fp_match,"%d 预测%s\n", gl_info.val_store[t_ab.row[0] + t_ab.val_offset], string_pre[t_ab.next_pre]);
				for (j = 0; j < FULL_LINE; j++)
				{
					tmp = data_pattern[tmp_pos + j];
					if (tmp == 10)
						fwrite(num_10, 1, 2, fp_match);
					else
					{
						tmp = tmp + '0';
						fwrite(&tmp, 1, 1, fp_match);
					}
					fwrite(&tab, 1, 1, fp_match);
				}
				fwrite(new_line, 1, 2, fp_match);
				fflush(fp_match);
				break;
			}

			if (_kbhit())
			{
				printf("\n");
				return 0;
			}
				
		}
	}

	return 1;

}


void read_num(char *from_str, char *to_str)
{
	while (*from_str != 0x0d && *(from_str + 1) != 0x0a)
		*to_str++ = *from_str++;
}

char item_table_en[][30] = {
	"total number:",
	"count uper limit:" ,
	"count lower limit:",
	"count limit:" ,
};

char item_table_ch[][30] = {
	"总数:",
	"循环上限:",
	"循环下限:",
	"计数上限:",
};

void get_cfg(FILE *fp)
{
	FILE *p_f = fp;
	char tmp[500] = { 0 };
	char num[10] = { 0 };
	int pos,i;

	int *p = (int *)(&cfg_info);
	fread(tmp, 1, 500, p_f);

	for (i = 0; i < 4; i++)
	{
		memset(num, 0, 10);
		pos = GetSubStrPos(tmp, item_table_en[i]);
		if (pos != -1)
		{
			read_num(&tmp[pos + strlen(item_table_en[i])], num);
			*p = atoi(num);
			printf("%s %d\n", item_table_ch[i], *p++);
		}
	}
}

int main(int argc, _TCHAR* argv[])
{
	int input_buff[10];
	char table[] = { "0123456789" };
	int i, j, tmp;

	fp_cfg = fopen("config.txt", "rb+");//打开cfg
	if (fp_cfg == NULL)
	{
		printf("no config file!\n");
		getchar();
		return 0;
	}
	get_cfg(fp_cfg);
	fclose(fp_cfg);//关掉cfg

	fp_data = fopen("data2.txt", "rb");
	if (fp_data == NULL)
	{
		printf("no data faile!\n");
		getchar();
		return 0;
	}
	fclose(fp_data);

	while (1)
	{
		memset(&gl_info, 0, sizeof(gl_info_t));
		memset(data_pattern, 0, sizeof(data_pattern));

		fp = fopen("game.txt", "rb+");
		if (fp == NULL)
		{
			printf("no data file!\n");
			getchar();
			return 0;
		}

		srand((int)time(NULL));     //时间产生种子

		fp_match = fopen("match.txt", "w");
		if (fp_match == NULL)
		{
			printf("match result file faile!\n");
			getchar();
			return 0;
		}

		fp_result = fopen("result.txt", "w");
		if (fp_result == NULL)
		{
			printf("result file faile!\n");
			getchar();
			return 0;
		}

		get_data_val(fp);
		round_buffer();

		printf("--------当前总期数 %d---------\n", gl_info.index);
		fprintf(fp_match,"--------当前总期数 %d---------\n", gl_info.index);
		/*
		process_data_val(10);
		printf("--------正序结果---------\n");
		fprintf(fp_match, "--------正序结果---------\n");

		if (find_run() == 0)
			printf("终止当前运算\n");
		printf("--------逆序结果---------\n");
		fprintf(fp_match, "--------逆序结果---------\n");
		for (i = 0; i<cfg_info.total / 2; i++)
		{
			tmp = data_pattern[i];
			data_pattern[i] = data_pattern[cfg_info.total - 1 - i];
			data_pattern[cfg_info.total - 1 - i] = tmp;
		}
		*/
		ts_run(tests);

		//if (find_run() == 0)
			//printf("终止当前运算\n");

		printf("请依次输入新数据\n");
		for (i = 0; i<10; i++)
		{
			scanf("%d", &input_buff[i]);
		}
		printf("您输入的数据是：");
		for (i = 0; i<10; i++)
		{
			printf("%3d", input_buff[i]);
		}
		printf("\n");

		for (i = 0; i<10; i++)
		{
			if (input_buff[i] != 10)
			{
				fwrite(&table[input_buff[i]], 1, 1, fp);
			}
			else
			{
				fwrite("10", 1, 2, fp);
			}
			if (i != 9)
				fwrite(&blank, 1, 1, fp);
		}
		fwrite(&new_line, 1, 2, fp);


		fclose(fp_match);
		fclose(fp_result);
		fclose(fp);

	}

	getchar();
	return 0;
}
