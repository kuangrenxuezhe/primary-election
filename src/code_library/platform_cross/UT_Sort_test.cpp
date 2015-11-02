// UT_Sort_test.h

#include "UH_Define.h"
#include "UT_Sort.h"
#include "UC_MD5.h"

/*
��·����  3162   3319   3232   3261   3197
����      8570   8959   8866   8893   8994
1k�ݹ�    16869  16900  16612  17062  16713
1k�ǵݹ�  16263  16481  16438  16560  16535
2k�ݹ�    19270  19421  19431  19476  19241
2k�ǵݹ�  21227  21294  21310  21059  21198
*/

/************************************************************************
  quicksort   compare_key    time(ms)
  ����        ����           292205         
  ����        ������         367819
  ������      ����           250909
  ������      ������         270654

  ���ۣ�quicksort���Ӳ������û����������½�(9)��compare_key���Ӳ������û������������(-2)��
  �����������ӻ�����۲죬���Ӳ������û������ڴ�����������ᵼ�������½���
  ���ǣ�ʹ��-O3���������Ż����ڱ����ò������غ��ٱ�����ʱ�ᵼ���������,
				  ���磬quicksort�������õݹ鷵�غ������ʹ�ã���compare_key�������ú������غ�
				  �������ᱻ��һ��ʹ��                                                                 
************************************************************************/

/*
var_4 main(var_4 argc, var_1* argv[])
{
	var_1 input[1024];

	printf("\n*** quick sort test ***\n");
	
	printf("-> please input load num: ");
	gets(input);
	var_4 num = (var_4)atol(input);
	printf("<- accept num %d\n", num);
	
	var_u4* page_id = new var_u4[num];
	var_u4* mark_id = new var_u4[num];
		
	var_u4* tmp_page = new var_u4[num];
	var_u4* tmp_mark = new var_u4[num];

	var_u4* tmp_dest = new var_u4[num];

	UT_Parallel_QS<var_u4> pqs;
	pqs.init_qs();	

	UC_MD5 md5;

	printf("-- create rand id\n");
//	for(var_4 i = 0; i < num; i++)
//	{
//		var_4 len = sprintf(input, "%d", i);
//		tmp_page[i] = md5.MD5Bits64((var_u1*)input, len);		
//	}
	cp_generate_random<var_u4>(tmp_page, num);
	cp_generate_random<var_u4>(tmp_mark, num);
	printf("-- rand id ok, total %d\n\n", num);

	for(;;)
	{
		printf("-- function list\n");
		printf("1. qs_recursion_1k\n");
		printf("2. qs_recursion_2k\n");

		printf("3. qs_unrecursion_1k\n");
		printf("4. qs_unrecursion_2k\n");

		printf("5. radix_sort_one_byte\n");
		printf("6. ut_parallel_qs\n");

		printf("7. qs_unrecursion_tk\n");

		printf("\n-> please choice function: ");
		gets(input);
		var_4 choice = (var_4)atol(input);
		printf("<- accept function %d\n\n", choice);
	
		memcpy(page_id, tmp_page, num * sizeof(var_u4));
		memcpy(mark_id, tmp_mark, num * sizeof(var_u4));

		printf("-- now sort, waiting ...\n");

		CP_STAT_TIME time;	
		time.set_time_begin();

		if(choice == 1)
			qs_recursion_1k<var_u4>(0, num - 1, page_id);
		else if(choice == 2)
			qs_recursion_2k<var_u4, var_u4>(0, num - 1, page_id, mark_id);
		else if(choice == 3)
			qs_unrecursion_1k<var_u4>(0, num - 1, page_id);
		else if(choice == 4)
			qs_unrecursion_2k<var_u4, var_u4>(0, num - 1, page_id, mark_id);
		else if(choice == 5)
			radix_sort_one_byte<var_u4>(num, page_id, tmp_dest);
		else if(choice == 6)
			pqs.data_qs(0, num - 1, page_id);
        else
		{
			printf("-- choice function invalid\n");
			return 0;
		}

		time.set_time_end();

		for(var_4 i = 1; i < num; i++)
		{
			if(page_id[i - 1] >= page_id[i])
			{
				printf("-- check sort result error\n\n");
				return -1;
			}
		}
		printf("-- check sort result success\n\n");
	}

	return 0;
}
*/
