#include "AnalyseConf.h"
#include <iostream>
#include <fstream>
using namespace std;
void analyse_conf()
{
	ofstream log("log.txt");

	char str[100];
	ifstream fin("conf.ini");
	fin.getline(str,1000);
	if(!strcmp(str,"[code2index]"))
	{
		fin.getline(str,sizeof(str));
		while(fin)
		{
			analyse_line(str);
			fin.getline(str,sizeof(str));
		}
	}

	log.close();
}
void analyse_line(char * str)
{
	ofstream log("log.txt",ios::app);

	if(strlen(str) == 0){
		cout << endl;
		return;
	}
	if( *str == '[' ){
		for(int i=0; i<strlen(str); i++)
			cout << *(str+i);
		cout << endl;
		return;
	}

	int i = 0, num = 0, audio_num_line = 0, name_i, repeat_flag=0;
	char temp_name[50];							//��ʱ�洢������Ϣ���������

	while( *(str+i) == ' ' )	i++;		//�ҵ�codeֵ��������ڿո�	
	code2index[var_code].code = atoi(str);	//��char�͵���ֵת��Ϊint	
	for(int i=0; i<var_code; i++)
		if(code2index[i].code == code2index[var_code].code)
		{
			cout << "error:�����ظ�����\"" << code2index[var_code].code << "\"��Ϊcodeֵ��" << endl;
			cerr << "error:�����ظ�����\"" << code2index[var_code].code << "\"��Ϊcodeֵ��" << endl;
			system("pause");
			ShellExecute(NULL,"open","clean.bat",NULL,NULL,SW_HIDE);
			exit(1);
		}

	cout << "--------------------------------------code2index 's code:" << code2index[var_code].code << endl;
	int j=0;
	num = code2index[var_code].code;
	while( num / 10)									//�ж�code��λ��(j+1)
	{
		j++;
		num /= 10;
	}
	i += (j+1);
	cout << "��Ƶ���ֱַ�Ϊ:" << endl;

	int break_flag = 0;
	while( i < strlen(str) )
	{
		while( *(str+i) == '=' || *(str+i) == ' ' || *(str+i) == '+'){ 		//�ҵ��ļ���
			i++;
			if(i >= strlen(str)){		//��⵽��β
				break_flag = 1;
			}
		}
		if(break_flag)
			break;

		j = 0;
		memset(temp_name,NULL,sizeof(temp_name));
		while( (*(str+i) != ' ') && (*(str+i) != '+') && (i < strlen(str)))		//ȡ���ļ���
		{
			temp_name[j] = *(str+i);
			i++;
			j++;
		}

		int z = 0;
		while(temp_name[z] != '.')
		{
			code2index[var_code].link_name[audio_num_line][z] = temp_name[z];
			z++;
		}
		strcat(code2index[var_code].link_name[audio_num_line],".s7");		//�滻�ļ���Ϊs7��ʽ

		cout << "��" << var_code << "��code�ĵ�" << audio_num_line << "����Ƶ�ǣ�" << temp_name << endl;
		for(name_i=0; name_i < audio_num_all; name_i++)
		{
			if(!strcmp(audio[name_i].audio_name,temp_name))
			{
				repeat_flag = 1;
				audio_num_repeat++;
				cout << "---�Ѿ����ڵ���Ƶ" << audio[name_i].audio_name << endl;
				break;
			}
		}
		if(repeat_flag == 0)
		{
			strcpy(audio[audio_num_all].audio_name,temp_name);
			cout << "-----------�¼ӵ���Ƶ" << audio[audio_num_all].audio_name << endl;
			audio_num_all++;
			cout << "�ܵ���Ƶ����" << audio_num_all << endl;
		}else{
			repeat_flag = 0;
		}
		audio_num_line++;
	}
	cout << "------------------------------------һ���е���Ƶ��������" << audio_num_line << endl;
	code2index[var_code].link_num = audio_num_line;
	var_code++;	
	log.close();
}