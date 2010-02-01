/*
 * Copyright (c) from 2000 to 2009
 * 
 * Network and System Laboratory 
 * Department of Computer Science 
 * College of Computer Science
 * National Chiao Tung University, Taiwan
 * All Rights Reserved.
 * 
 * This source code file is part of the NCTUns 6.0 network simulator.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted (excluding for commercial or
 * for-profit use), provided that both the copyright notice and this
 * permission notice appear in all copies of the software, derivative
 * works, or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation, and that credit
 * is given to National Chiao Tung University, Taiwan in all publications 
 * reporting on direct or indirect use of this code or its derivatives.
 *
 * National Chiao Tung University, Taiwan makes no representations 
 * about the suitability of this software for any purpose. It is provided 
 * "AS IS" without express or implied warranty.
 *
 * A Web site containing the latest NCTUns 6.0 network simulator software 
 * and its documentations is set up at http://NSL.csie.nctu.edu.tw/nctuns.html.
 *
 * Project Chief-Technology-Officer
 * 
 * Prof. Shie-Yuan Wang <shieyuan@csie.nctu.edu.tw>
 * National Chiao Tung University, Taiwan
 *
 * 09/01/2009
 */

#include "WSM.h"
#include <string.h>

void sig_usr1(int signo)
{
	sigprocmask(SIG_BLOCK, &usr1_mask, NULL);
	if(WSM_ahead_recv > 0)// packet has already receive by GetCurrentTime
	{
		WSM_ahead_recv --;
		return;
	}
	char *mesg;
	mesg = recvWSM(myTCPsockfd, recv_wsmp);
	if(mesg != NULL)
	{
		free(mesg);
	}
	sigprocmask(SIG_UNBLOCK, &usr1_mask, NULL);
}

int sendWSM(int myTCPsockfd, struct WaveShortMessageProtocol send_wsmp, char *wsm_data)
{
	sigprocmask(SIG_BLOCK, &usr1_mask, NULL);
	char *msg;
	int n;

	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	memcpy(msg, &send_wsmp, sizeof(struct WaveShortMessageProtocol));
	memcpy( (msg+sizeof(struct WaveShortMessageProtocol)), wsm_data, send_wsmp.wsm_header.wsm_len);

	n = writen(myTCPsockfd, msg, sizeof(struct WaveShortMessageProtocol) + send_wsmp.wsm_header.wsm_len);
	if(n < 0)
	{
		perror("WSM Write Packet Error and exit\n");
		exit(0);
	}

	free(msg);
	sigprocmask(SIG_UNBLOCK, &usr1_mask, NULL);
	return n;
}

char* recvWSM(int myTCPsockfd, struct WaveShortMessageProtocol recvWSMPheader)
{
	int n;
	char *msg;
	struct typeChecker p;
	n = recv(myTCPsockfd, (char *) &p, sizeof(struct typeChecker), MSG_PEEK);
	if(n <= 0)
	{
		printf("recv packet error or server disconnect and exit\n");
		exit(0);
	}
	while(p.type != WAVE_SHORT_MESSAGE_PROTOCOL)
	{
		if(p.type == GET_SE_TIME)
		{
			printf("recv error message type from SE: type is WAVE_SHORT_MESSAGE_PROTOCOL\n");
			exit(1);
		}
		else
		{
			printf("recv error message type from SE %u\n", p.type);
			exit(1);
		}
		//n = recv(myTCPsockfd, (char *) &p, sizeof(struct typeChecker), MSG_PEEK);
		//if(n <= 0)
		//{
		//	printf("recv packet error or server disconnect and exit\n");
		//	exit(0);
		//	}
	}
	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);

	n = readn(myTCPsockfd, msg, sizeof(struct WaveShortMessageProtocol));
	if((n != sizeof(struct WaveShortMessageProtocol)) || (((struct WaveShortMessageProtocol*) msg)->type != WAVE_SHORT_MESSAGE_PROTOCOL))
	{
		printf("IPC WAVE_SHORT_MESSAGE_PROTOCOL failed.\n");
		delete msg;
		exit(0);
	}
	memcpy(&recvWSMPheader, msg, sizeof(struct WaveShortMessageProtocol));
	n = readn(myTCPsockfd, msg, recvWSMPheader.wsm_header.wsm_len);
	printf("[WSM %d] recv %d bytes from PSID(%d)\n", mynid, n, recvWSMPheader.wsm_header.psid);
	return msg;
}

int enableReginfo(int myTCPsockfd, struct WaveAgentEnableRegInfo wave_enable_reg_info)
{
	char *msg;
	msg = (char *) malloc(sizeof(struct WaveAgentEnableRegInfo));
	memcpy(msg, &wave_enable_reg_info, sizeof(struct WaveAgentEnableRegInfo));
	writen(myTCPsockfd, msg, sizeof(struct WaveAgentEnableRegInfo));
	free(msg);
	return 1;
}

u_int64_t getCurrentTime(int myTCPsockfd, int moreMsgFollowing)
{	
	char *msg;
	struct typeChecker p;
	int n;

	sigprocmask(SIG_BLOCK, &usr1_mask, NULL);

	msg = (char *) malloc(sizeof(struct GetSETime));
	get_se_time.moreMsgFollowing = moreMsgFollowing;
	get_se_time.nid = mynid;
	memcpy(msg, &get_se_time, sizeof(struct GetSETime));
	n = writen(myTCPsockfd, msg, sizeof(struct GetSETime));
	if(n < 0)
	{
		perror("WSM Write Packet Error and exit");
		exit(0);
	}
	n = recv(myTCPsockfd, (char *) &p, sizeof(struct typeChecker), MSG_PEEK);
	if(n <= 0)
	{
		printf("recv packet error or server disconnect and exit\n");
		exit(0);
	}

	while(p.type != GET_SE_TIME)
	{
		if(p.type == WAVE_SHORT_MESSAGE_PROTOCOL)
		{
			WSM_ahead_recv ++;
			char *mesg = recvWSM(myTCPsockfd, recv_wsmp);
			if(mesg != NULL)
				free(mesg);
		}
		else
		{
			printf("recv error message type from SE %u\n", p.type);
			exit(1);
		}
		n = recv(myTCPsockfd, (char *) &p, sizeof(struct typeChecker), MSG_PEEK);
		if(n <= 0)
		{
			printf("recv packet error or server disconnect and exit\n");
			exit(0);
		}
	}
	n = readn(myTCPsockfd, msg, sizeof(struct GetSETime));
	if((n != sizeof(struct GetSETime)) || (((struct GetSETime*) msg)->type != GET_SE_TIME))
	{
		printf("IPC GET_SE_TIME failed.\n");
		delete msg;
		exit(0);
	}
	memcpy(&get_se_time, msg, sizeof(struct GetSETime));
	free(msg);

	sigprocmask(SIG_UNBLOCK, &usr1_mask, NULL);

	return get_se_time.time;
}

void init()
{
	AppWsmData = (char *) malloc(sizeof(char) * WsmMaxLength);	
	mynid = getMyNodeID();
	pid = getpid();
	myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine( mynid, 1, -1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_WSM, 1);

	wave_enable_reg_info.type		= WAVE_AGENT_ENABLE_REG_INFO;
	wave_enable_reg_info.moreMsgFollowing	= 1;
	wave_enable_reg_info.nid		= mynid;
	wave_enable_reg_info.pid		= pid;
	enableReginfo(myTCPsockfd, wave_enable_reg_info);

	my_wsmp.type 			= WAVE_SHORT_MESSAGE_PROTOCOL;
	my_wsmp.moreMsgFollowing 	= 1;
	my_wsmp.nid 			= mynid;
	//set WSM header
	my_wsmp.wsm_header.wsm_version	= 1;
	my_wsmp.wsm_header.header_len	= 9;	//excluding WSM version, Header Length, WSM Length, WSM Data
	my_wsmp.wsm_header.header_cont	= 0;
	my_wsmp.wsm_header.security_type= 0;
	my_wsmp.wsm_header.channel_num	= 0;
	my_wsmp.wsm_header.data_rate	= 0;
	my_wsmp.wsm_header.txpwr_level	= 0;
	my_wsmp.wsm_header.psid		= 0;
	my_wsmp.wsm_header.expiry_time	= 0;
	my_wsmp.wsm_header.wsm_len	= 1400;
	
	get_se_time.type	= GET_SE_TIME;

	WSM_ahead_recv = 0;
	sigemptyset(&usr1_mask);
	sigaddset(&usr1_mask, SIGUSR1);
}

int main(int argc, char *argv[])
{
	init();
	signal(SIGUSR1, sig_usr1);
	u_int64_t b_sleep_t, a_sleep_t;
	int remainTime;
	if(argc != 2)
	{
		printf("usage: WSM [packet send period(ms)]\n");
		printf("[WSM %d]: use default packet_send_period(1 sec)\n", mynid);
		packet_send_period = AppSendMesgInterval;
	}
	else
	{
		if(atoi(argv[1]) != 0)
		{
			packet_send_period = atoi(argv[1]);
			printf("[WSM %d]: packet_send_period = %d(ms)\n", mynid, packet_send_period);
		}
		else
		{
			printf("[WSM %d]: bed parameter ,use default packet_send_period(1 sec)\n", mynid);
			packet_send_period = AppSendMesgInterval;
		}
	}


	while(1)
	{
		sendWSM(myTCPsockfd, my_wsmp, AppWsmData);
		b_sleep_t = getCurrentTime(myTCPsockfd, 1);
		//usleepAndReleaseCPU(myTCPsockfd, mynid, CMD_SERVER_TRIGER, 0);
		usleepAndReleaseCPU(myTCPsockfd, mynid, packet_send_period, 0);
		a_sleep_t = getCurrentTime(myTCPsockfd, 1);
		remainTime = (a_sleep_t-b_sleep_t)/10;
		while( remainTime < packet_send_period)
		{
			getCurrentTime(myTCPsockfd, 1);
			//if((packet_send_period - remainTime) > CMD_SERVER_TRIGER)
			//	usleepAndReleaseCPU(myTCPsockfd, mynid, CMD_SERVER_TRIGER, 1);
			//else
			//	usleepAndReleaseCPU(myTCPsockfd, mynid, (AppSendMesgInterval - remainTime), 1);
			usleepAndReleaseCPU(myTCPsockfd, mynid, (packet_send_period - remainTime), 1);
			a_sleep_t = getCurrentTime(myTCPsockfd, 1);
			remainTime = (a_sleep_t-b_sleep_t)/10;
		}
	}
}

