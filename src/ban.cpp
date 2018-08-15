/* ************************************************************************
*   File: ban.cpp                                       Part of Bylins    *
*  Usage: banning/unbanning/checking sites and player names               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "ban.hpp"
#include "char.hpp"
#include "modify.h"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "global.objects.hpp"
#include "diskio.h"

#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>

void do_ban(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (!*argument)
	{
		ban->ShowBannedIp(BanList::SORT_BY_DATE, ch);
		return;
	}

	char flag[MAX_INPUT_LENGTH], site[MAX_INPUT_LENGTH];
	argument = two_arguments(argument, flag, site);

	if (!str_cmp(flag, "proxy"))
	{
		if (!*site)
		{
			ban->ShowBannedProxy(BanList::SORT_BY_NAME, ch);
			return;
		}
		if (site[0] == '-')
			switch (site[1])
			{
			case 'n':
			case 'N':
				ban->ShowBannedProxy(BanList::SORT_BY_BANNER, ch);
				return;
			case 'i':
			case 'I':
				ban->ShowBannedProxy(BanList::SORT_BY_NAME, ch);
				return;
			default:
				send_to_char("Usage: ban proxy [[-N | -I] | ip] \r\n", ch);
				send_to_char(" -N : Sort by banner name \r\n", ch);
				send_to_char(" -I : Sort by banned ip \r\n", ch);
				return;
			};
		const std::string banned_ip(site);
		const std::string banner_name(GET_NAME(ch));

		if (!ban->add_proxy_ban(banned_ip, banner_name))
		{
			send_to_char("The site is already in the proxy ban list.\r\n", ch);
			return;
		}
		send_to_char("Proxy banned.\r\n", ch);
		return;
	}

	if (!*site && flag[0] == '-')
		switch (flag[1])
		{
		case 'n':
		case 'N':
			ban->ShowBannedIp(BanList::SORT_BY_BANNER, ch);
			return;
		case 'd':
		case 'D':
			ban->ShowBannedIp(BanList::SORT_BY_DATE, ch);
			return;
		case 'i':
		case 'I':
			ban->ShowBannedIp(BanList::SORT_BY_NAME, ch);
			return;
		default:
			;
		};


	if (!*flag || !*site)
	{
		send_to_char("Usage: ban [[-N | -D | -I] | {all | select | new } ip duration [reason]] \r\n", ch);
		send_to_char("or\r\n", ch);
		send_to_char(" ban proxy [[-N | -I] | ip] \r\n", ch);
		send_to_char(" -N : Sort by banner name \r\n", ch);
		send_to_char(" -D : Sort by ban date \r\n", ch);
		send_to_char(" -I : Sort by bannd ip \r\n", ch);
		return;
	}

	if (!(!str_cmp(flag, "select") || !str_cmp(flag, "all")
			|| !str_cmp(flag, "new")))
	{
		send_to_char("Flag must be ALL, SELECT, or NEW.\r\n", ch);
		return;
	}

	char length[MAX_INPUT_LENGTH], *reason;
	int len, ban_type = BanList::BAN_ALL;
	reason = one_argument(argument, length);
	skip_spaces(&reason);
	len = atoi(length);
	if (!*length || len == 0)
	{
		send_to_char("Usage: ban {all | select | new } ip duration [reason]\r\n", ch);
		return;
	}
	const std::string banned_ip(site);
	const std::string banner_name(GET_NAME(ch));
	const std::string ban_reason(reason);
	for (int i = BanList::BAN_NEW; i <= BanList::BAN_ALL; i++)
		if (!str_cmp(flag, BanList::ban_types[i]))
			ban_type = i;


	if (!ban->add_ban(banned_ip, ban_reason, banner_name, len, ban_type))
	{
		send_to_char("That site has already been banned -- unban it to change the ban type.\r\n", ch);
		return;
	}
	send_to_char("Site banned.\r\n", ch);
}

void do_unban(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char site[MAX_INPUT_LENGTH];
	one_argument(argument, site);
	if (!*site)
	{
		send_to_char("A site to unban might help.\r\n", ch);
		return;
	}
	const std::string unban_site(site);
	if (!ban->unban(unban_site, ch))
	{
		send_to_char("The site is not currently banned.\r\n", ch);
		return;
	}
}


/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

#define MAX_INVALID_NAMES	200

char *invalid_list[MAX_INVALID_NAMES];
int num_invalid = 0;

int Is_Valid_Name(char *newname)
{
	int i;
	char tempname[MAX_INPUT_LENGTH];

	if (!*invalid_list || num_invalid < 1)
		return (1);

	// change to lowercase
	strcpy(tempname, newname);
	for (i = 0; tempname[i]; i++)
		tempname[i] = LOWER(tempname[i]);

	// Does the desired name contain a string in the invalid list?
	for (i = 0; i < num_invalid; i++)
		if (strstr(tempname, invalid_list[i]))
			return (0);

	return (1);
}


int Is_Valid_Dc(char *newname)
{
	DESCRIPTOR_DATA *dt;

	for (dt = descriptor_list; dt; dt = dt->next)
		if (dt->character && GET_NAME(dt->character)
				&& !str_cmp(GET_NAME(dt->character), newname))
			return (0);
	return (1);
}

int Valid_Name(char *newname)
{
	return (Is_Valid_Name(newname) && Is_Valid_Dc(newname));
}



void Read_Invalid_List(void)
{
	FILE *fp;
	char temp[256];

	if (!(fp = fopen(XNAME_FILE, "r")))
	{
		perror("SYSERR: Unable to open '" XNAME_FILE "' for reading");
		return;
	}

	num_invalid = 0;
	while (get_line(fp, temp) && num_invalid < MAX_INVALID_NAMES)
		invalid_list[num_invalid++] = str_dup(temp);

	if (num_invalid >= MAX_INVALID_NAMES)
	{
		log("SYSERR: Too many invalid names; change MAX_INVALID_NAMES in ban.c");
		exit(1);
	}

	fclose(fp);
}

// ��� ���, ��� ������� � ������������ ������ � �.�.

// ����������� ���������� ip � ����� ��� ���������
unsigned long TxtToIp(const char * text)
{
	int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
	unsigned long ip = 0;

	sscanf(text, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
	ip = (ip1 << 24) + (ip2 << 16) + (ip3 << 8) + ip4;
	return ip;
}

// ���� ��� ���������� ���������
unsigned long TxtToIp(std::string &text)
{
	int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
	unsigned long ip = 0;

	sscanf(text.c_str(), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
	ip = (ip1 << 24) + (ip2 << 16) + (ip3 << 8) + ip4;
	return ip;
}

// ���� �����, ������ ������� ��� ����������� �����������
std::string IpToTxt(unsigned long ip)
{
	char text[16];
	sprintf(text, "%lu.%lu.%lu.%lu", ip >> 24, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
	std::string buffer = text;
	return buffer;
}

#define MAX_PROXY_CONNECT 50

struct ProxyIp
{
	unsigned long ip2;   // �������� �� � ������ ���������
	int num;             // ���-�� ������������ ���������
	std::string text;    // �����������
	std::string textIp;  // �� � ���� ������
	std::string textIp2; // �������� �� � ���� ������
};

typedef std::shared_ptr<ProxyIp> ProxyIpPtr;
typedef std::map<unsigned long, ProxyIpPtr> ProxyListType;
// ���������� ������ �� � ���-�� ���������
static ProxyListType proxyList;

// ���������� ������ ������ � ���� (�� � ��������� ����, ��� ������ ����� ���� � �����)
void SaveProxyList()
{
	std::ofstream file(PROXY_FILE);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", PROXY_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	for (ProxyListType::const_iterator it = proxyList.begin(); it != proxyList.end(); ++it)
	{
		file << it->second->textIp << "  " << (it->second->textIp2.empty() ? "0"
											   : it->second->textIp2) << "  " << it->second->num << "  " << it->second->text << "\n";
	}
	file.close();
}

// ���� ������ ip � ����������� ����������� ��������� � ���
void LoadProxyList()
{
	// ���� ��������
	proxyList.clear();

	std::ifstream file(PROXY_FILE);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", PROXY_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer, textIp, textIp2;
	int num = 0;

	while (file)
	{
		file >> textIp >> textIp2 >> num;
		std::getline(file, buffer);
		boost::trim(buffer);
		if (textIp.empty() || textIp2.empty() || buffer.empty() || num < 2 || num > MAX_PROXY_CONNECT)
		{
			log("Error read file: %s! IP: %s IP2: %s Num: %d Text: %s (%s %s %d)", PROXY_FILE, textIp.c_str(),
				textIp2.c_str(), num, buffer.c_str(), __FILE__, __func__, __LINE__);
			// �� ����� ������� ����, ���� ��� ��� �� �� - ��������� ����� ��� ����������
			// ���� ����� ������, ���� ����� ���������� ����� ���-�� ������� ������ � ������� �)
			proxyList.clear();
			return;
		}

		ProxyIpPtr tempIp(new ProxyIp);
		tempIp->num = num;
		tempIp->text = buffer;
		tempIp->textIp = textIp;
		// 0 ��� ������, ��� ��� �� ��������, ����� �� ������� ��� ������ � ���� - ����� ������ ������
		if (textIp2 != "0")
			tempIp->textIp2 = textIp2;
		unsigned long ip = TxtToIp(textIp);
		tempIp->ip2 = TxtToIp(textIp2);
		proxyList[ip] = tempIp;
	}
	file.close();
	SaveProxyList();
}

// �������� �� ����������� � ������ ������������������ ip � ���-�� ����������
// 0 - ����, 1 - � ���� ��� ����. ����� ��������� � ������� ip, 2 - ��� ��
int CheckProxy(DESCRIPTOR_DATA * ch)
{
	//������� ���� � ������, ����� ��� �������� �������
	ProxyListType::const_iterator it;
	// ����� ������� - ���� ������ ��, ���� ������� ��������, ���� - ���� ������� �� ��������� ��
	for (it = proxyList.begin(); it != proxyList.end(); ++it)
	{
		if (!it->second->ip2)
		{
			if (it->first == ch->ip)
				break;
		}
		else
			if ((it->first <= ch->ip) && (ch->ip <= it->second->ip2))
				break;
	}
	if (it == proxyList.end())
		return 0;

	// ���� ����� � ���������
	DESCRIPTOR_DATA *i;
	int num_ip = 0;
	for (i = descriptor_list; i; i = i->next)
		if (i != ch && i->character && !IS_IMMORTAL(i->character) && (i->ip == ch->ip))
			num_ip++;

	// ��������� �� ���-�� ���������
	if (it->second->num <= num_ip)
		return 1;

	return 2;
}

// ������� proxy
void do_proxy(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "list") || CompareParam(buffer2, "������"))
	{
		boost::format proxyFormat(" %-15s   %-15s   %-2d   %s\r\n");
		std::ostringstream out;
		out << "������ ������: IP | IP2 | �������� ���������� | �����������\r\n";

		for (ProxyListType::const_iterator it = proxyList.begin(); it != proxyList.end(); ++it)
			out << proxyFormat % it->second->textIp % it->second->textIp2 % it->second->num % it->second->text;

		page_string(ch->desc, out.str());

	}
	else if (CompareParam(buffer2, "add") || CompareParam(buffer2, "��������"))
	{
		GetOneParam(buffer, buffer2);
		if (buffer2.empty())
		{
			send_to_char("������� �������������� IP ����� ��� �������� mask|�����.\r\n", ch);
			return;
		}
		// ������ ���������� ��������� ��
		std::string textIp, textIp2;
		if (CompareParam(buffer2, "mask") || CompareParam(buffer2, "�����"))
		{
			GetOneParam(buffer, buffer2);
			if (buffer2.empty())
			{
				send_to_char("������� ������ ���������.\r\n", ch);
				return;
			}
			textIp = buffer2;
			// ������ ���� ������ ��
			GetOneParam(buffer, buffer2);
			if (buffer2.empty())
			{
				send_to_char("������� ����� ���������.\r\n", ch);
				return;
			}
			textIp2 = buffer2;
		}
		else // ���� ��� �� �������� � �� ������ ������ - ������ ��� ������� ��
			textIp = buffer2;

		GetOneParam(buffer, buffer2);
		if (buffer2.empty())
		{
			send_to_char("������� ������������ ���-�� ���������.\r\n", ch);
			return;
		}
		const int num = atoi(buffer2.c_str());
		if (num < 2 || num > MAX_PROXY_CONNECT)
		{
			send_to_char(ch, "������������ ����� ��������� (2-%d).\r\n", MAX_PROXY_CONNECT);
			return;
		}

		boost::trim_if(buffer, boost::is_any_of(std::string(" \'")));
		if (buffer.empty())
		{
			send_to_char("������� ������� �����������.\r\n", ch);
			return;
		}
		char timeBuf[11];
		time_t now = time(0);
		strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&now));

		ProxyIpPtr tempIp(new ProxyIp);
		tempIp->text = buffer + " [" + GET_NAME(ch) + " " + timeBuf + "]";
		tempIp->num = num;
		tempIp->textIp = textIp;
		tempIp->textIp2 = textIp2;
		const unsigned long ip = TxtToIp(textIp);
		const unsigned long ip2 = TxtToIp(textIp2);
		if (ip2 && ((ip2 - ip) > 65535))
		{
			send_to_char("������� ������� ��������. (�������� x.x.0.0. �� x.x.255.255).\r\n", ch);
			return;
		}
		tempIp->ip2 = ip2;
		proxyList[ip] = tempIp;
		SaveProxyList();
		send_to_char("������ ���������.\r\n", ch);

	}
	else if (CompareParam(buffer2, "remove") || CompareParam(buffer2, "�������"))
	{
		GetOneParam(buffer, buffer2);
		if (buffer2.empty())
		{
			send_to_char("������� ��������� IP ����� ��� ������ ���������.\r\n", ch);
			return;
		}
		const unsigned long ip = TxtToIp(buffer2);
		ProxyListType::iterator it;
		it = proxyList.find(ip);
		if (it == proxyList.end())
		{
			send_to_char("������ IP/�������� � ��� �� ���������������.\r\n", ch);
			return;
		}
		proxyList.erase(it);
		SaveProxyList();
		send_to_char("������ �������.\r\n", ch);

	}
	else
		send_to_char("������: proxy <list|������>\r\n"
					 "        proxy <add|��������> ip <���������� ���������> �����������\r\n"
					 "        proxy <add|��������> <mask|�����> <������ ���������> <����� ���������> <���������� ���������> �����������\r\n"
					 "        proxy <remove|�������> ip\r\n", ch);
}

//////////////////////////////////////////////////////////////////////////////

const char *const
BanList::ban_filename = "etc/badsites";
//const char * const BanList::ban_filename        = "badsites";
//const char * const BanList::proxy_ban_filename  = "badproxy";
const char *const
BanList::proxy_ban_filename = "etc/badproxy";
const char *const
BanList::proxy_ban_filename_tmp = "etc/badproxy2";
const char *BanList::ban_types[] =
{
	"no",
	"new",
	"select",
	"all",
	"ERROR"
};

////////////////////////////////////////////////////////////////////////////////
bool BanList::ban_compare(BanNodePtr nodePtr, int mode, const void *op2)
{
	switch (mode)
	{
	case BAN_IP_COMPARE:
		if (nodePtr->BannedIp == *(std::string *) op2)
			return true;
		break;
	case BAN_TIME_COMPARE:
		if (nodePtr->BanDate == *(long *) op2)
			return true;
		break;
	default:
		;
	}

	return false;
}

bool BanList::proxy_ban_compare(ProxyBanNodePtr nodePtr, int mode, const void *op2)
{
	switch (mode)
	{
	case BAN_IP_COMPARE:
		if (nodePtr->BannedIp == *(std::string *) op2)
			return true;
		break;
	default:
		;
	}

	return false;
}


bool BanList::ban_sort_func(const BanNodePtr & lft, const BanNodePtr & rght, int sort_algorithm)
{
	switch (sort_algorithm)
	{
	case SORT_BY_NAME:
		return std::lexicographical_compare(lft->BannedIp.begin(),
											lft->BannedIp.end(), rght->BannedIp.begin(), rght->BannedIp.end());
	case SORT_BY_DATE:
		return (lft->BanDate > rght->BanDate);
	case SORT_BY_BANNER:
		return std::lexicographical_compare(lft->BannerName.begin(),
											lft->BannerName.end(),
											rght->BannerName.begin(), rght->BannerName.end());
	default:
		return true;
	}
}

bool BanList::proxy_ban_sort_func(const ProxyBanNodePtr & lft, const ProxyBanNodePtr & rght, int sort_algorithm)
{
	switch (sort_algorithm)
	{
	case SORT_BY_NAME:
		return std::lexicographical_compare(lft->BannedIp.begin(),
											lft->BannedIp.end(), rght->BannedIp.begin(), rght->BannedIp.end());
	case SORT_BY_BANNER:
		return std::lexicographical_compare(lft->BannerName.begin(),
											lft->BannerName.end(),
											rght->BannerName.begin(), rght->BannerName.end());
	default:
		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////
BanList::BanList() :
	current_sort_algorithm(0),
	current_proxy_sort_algorithm(0)
{

	//Do not fail if ban fiels weren't found, this is not critical at all
	/*
	if (!(reload_ban() && reload_proxy_ban(RELOAD_MODE_MAIN)))
	{
		log("Failed to initialize ban structures.\r\n");
		exit(0);
	}
	*/
	reload_ban() ;
	reload_proxy_ban(RELOAD_MODE_MAIN);
	sort_ip(SORT_BY_NAME);
	sort_proxy(SORT_BY_NAME);
	purge_obsolete();
}

void BanList::sort_ip(int sort_algorithm)
{
	if (sort_algorithm == current_sort_algorithm)
		return;

	Ban_List.sort([&](const BanNodePtr lhs, const BanNodePtr rhs)
	{
		return this->ban_sort_func(lhs, rhs, sort_algorithm);
	});

	current_sort_algorithm = sort_algorithm;
}

void BanList::sort_proxy(int sort_algorithm)
{
	if (sort_algorithm == current_proxy_sort_algorithm)
		return;

	Proxy_Ban_List.sort([&](const ProxyBanNodePtr lhs, const ProxyBanNodePtr rhs)
	{
		return this->proxy_ban_sort_func(lhs, rhs, sort_algorithm);
	});

	current_proxy_sort_algorithm = sort_algorithm;
}

bool BanList::add_ban(const std::string& BannedIp, const std::string& BanReason, const std::string& BannerName, int UnbanDate, int BanType)
{
	BanNodePtr temp_node_ptr(new struct BanNode);

	if (BannedIp.empty() || BannerName.empty())
		return false;

	// looking if the ip is already in the ban list
	const auto i =
		std::find_if(Ban_List.begin(), Ban_List.end(), [&](const BanNodePtr ptr)
	{
		return this->ban_compare(ptr, BAN_IP_COMPARE, &BannedIp);
	});

	if (i != Ban_List.end())
		return false;

	temp_node_ptr->BannedIp = BannedIp;
	temp_node_ptr->BanReason = (BanReason.empty() ? "Unknown" : BanReason);
	temp_node_ptr->BannerName = BannerName;
	temp_node_ptr->BanDate = time(0);
	temp_node_ptr->UnbanDate = (UnbanDate > 0) ? time(0) + UnbanDate * 60 * 60 : BAN_MAX_TIME;
	temp_node_ptr->BanType = BanType;

	// checking all strings: replacing all whitespaces with underlines
	size_t k = temp_node_ptr->BannedIp.size();
	for (size_t j = 0; j < k; j++)
	{
		if (temp_node_ptr->BannedIp[j] == ' ')
		{
			temp_node_ptr->BannedIp[j] = '_';
		}
	}

	k = temp_node_ptr->BannerName.size();
	for (size_t j = 0; j < k; j++)
	{
		if (temp_node_ptr->BannerName[j] == ' ')
		{
			temp_node_ptr->BannerName[j] = '_';
		}
	}

	k = temp_node_ptr->BanReason.size();
	for (size_t j = 0; j < k; j++)
	{
		if (temp_node_ptr->BanReason[j] == ' ')
		{
			temp_node_ptr->BanReason[j] = '_';
		}
	}

	Ban_List.push_front(temp_node_ptr);
	current_sort_algorithm = SORT_UNDEFINED;
	save_ip();
///////////////////////////////////////////////////////////////////////
	if (BanType == 3)
		disconnectBannedIp(BannedIp);

	sprintf(buf, "%s has banned %s for %s players(%s) (%dh).",
			BannerName.c_str(), BannedIp.c_str(), ban_types[BanType], temp_node_ptr->BanReason.c_str(), UnbanDate);
	mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
	imm_log("%s has banned %s for %s players(%s) (%dh).", BannerName.c_str(),
			BannedIp.c_str(), ban_types[BanType], temp_node_ptr->BanReason.c_str(), UnbanDate);

///////////////////////////////////////////////////////////////////////
	return true;
}

bool BanList::add_proxy_ban(const std::string& BannedIp, const std::string& BannerName)
{
	ProxyBanNodePtr temp_node_ptr(new struct ProxyBanNode);
	if (BannedIp.empty() || BannerName.empty())
		return false;

	const auto i =
		std::find_if(Proxy_Ban_List.begin(), Proxy_Ban_List.end(), [&](const ProxyBanNodePtr ptr)
	{
		return this->proxy_ban_compare(ptr, BAN_IP_COMPARE, &BannedIp);
	});

	if (i != Proxy_Ban_List.end())
		return false;

	temp_node_ptr->BannedIp = BannedIp;
	temp_node_ptr->BannerName = BannerName;

	// checking all strings: replacing all whitespaces with underlines
	size_t k = temp_node_ptr->BannedIp.size();
	for (size_t j = 0; j < k; j++)
	{
		if (temp_node_ptr->BannedIp[j] == ' ')
		{
			temp_node_ptr->BannedIp[j] = '_';
		}
	}

	k = temp_node_ptr->BannerName.size();
	for (size_t j = 0; j < k; j++)
	{
		if (temp_node_ptr->BannerName[j] == ' ')
		{
			temp_node_ptr->BannerName[j] = '_';
		}
	}

	Proxy_Ban_List.push_front(temp_node_ptr);
	current_proxy_sort_algorithm = SORT_UNDEFINED;
	save_proxy();
///////////////////////////////////////////////////////////////////////
	disconnectBannedIp(BannedIp);
	sprintf(buf, "%s has banned proxy %s", BannerName.c_str(), BannedIp.c_str());
	mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
	imm_log("%s has banned proxy %s", BannerName.c_str(), BannedIp.c_str());
///////////////////////////////////////////////////////////////////////
	return true;
}

bool BanList::reload_ban(void)
{
	typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
	FILE *loaded;
	Ban_List.clear();
	if ((loaded = fopen(ban_filename, "r")))
	{
		std::string str_to_parse;
		const boost::char_separator<char> sep(" ");

		DiskIo::read_line(loaded, str_to_parse, true);
		tokenizer tokens(str_to_parse, sep);

		tokenizer::iterator tok_iter = tokens.begin();
		// process header
		if (tok_iter != tokens.end())
		{
			if (++tok_iter == tokens.end())
			{
				DiskIo::read_line(loaded, str_to_parse, true);
				tokens.assign(str_to_parse);
			}
		}

		while (!feof(loaded))
		{
			BanNodePtr ptr(new struct BanNode);
			tok_iter = tokens.begin();
			// find ban_type
			if (tok_iter != tokens.end())
			{
				for (int i = BAN_NO; i <= BAN_ALL; i++)
					if (!strcmp((*tok_iter).c_str(), ban_types[i]))
						ptr->BanType = i;
			}
			else
			{
				DiskIo::read_line(loaded, str_to_parse, true);
				tokens.assign(str_to_parse);
				continue;
			}
			// ip
			if (++tok_iter != tokens.end())
			{
				ptr->BannedIp = (*tok_iter);
				// removing port specification i.e. 129.1.1.1:8080; :8080 is removed
				const auto at = ptr->BannedIp.find_first_of(':');
				if (at != std::string::npos)
					ptr->BannedIp.erase(at);
			}
			else
			{
				DiskIo::read_line(loaded, str_to_parse, true);
				tokens.assign(str_to_parse);
				continue;
			}
			//ban_date
			if (++tok_iter != tokens.end())
			{
				ptr->BanDate = atol((*tok_iter).c_str());
			}
			else
			{
				DiskIo::read_line(loaded, str_to_parse, true);
				tokens.assign(str_to_parse);
				continue;
			}

			//banner_name
			if (++tok_iter != tokens.end())
			{
				ptr->BannerName = (*tok_iter);
			}
			else
			{
				DiskIo::read_line(loaded, str_to_parse, true);
				tokens.assign(str_to_parse);
				continue;
			}

			//ban_length
			if (++tok_iter != tokens.end())
			{
				ptr->UnbanDate = atol((*tok_iter).c_str());
			}
			else
			{
				DiskIo::read_line(loaded, str_to_parse, true);
				tokens.assign(str_to_parse);
				continue;
			}
			//ban_reason
			if (++tok_iter != tokens.end())
			{
				ptr->BanReason = (*tok_iter);
			}
			else
			{
				DiskIo::read_line(loaded, str_to_parse, true);
				tokens.assign(str_to_parse);
				continue;
			}
			Ban_List.push_front(ptr);
			DiskIo::read_line(loaded, str_to_parse, true);
			tokens.assign(str_to_parse);
		}
		fclose(loaded);
		return true;
	}
	log("SYSERR: Unable to open banfile");
	return false;
}

bool BanList::reload_proxy_ban(int mode)
//mode:
//0 - load normal file
//1 - load temp file & merge with normal then kill it
{
	typedef boost::tokenizer < boost::char_separator < char > >tokenizer;
	FILE *loaded;

	if (mode == RELOAD_MODE_MAIN)
	{
		Proxy_Ban_List.clear();
		loaded = fopen(proxy_ban_filename, "r");
	}
	else if (mode == RELOAD_MODE_TMPFILE)
		loaded = fopen(proxy_ban_filename_tmp, "r");
	else
		return false;

	if (loaded)
	{
		std::string str_to_parse;
		const boost::char_separator<char> sep(" ");
		tokenizer tokens(str_to_parse, sep);

		while (DiskIo::read_line(loaded, str_to_parse, true))
		{
			tokens.assign(str_to_parse);
			auto tok_iter = tokens.begin();
			ProxyBanNodePtr ptr(new struct ProxyBanNode);
			ptr->BannerName = "Undefined";
			if (tok_iter != tokens.end())
			{
				ptr->BannedIp = (*tok_iter);
				// removing port specification i.e. 129.1.1.1:8080; :8080 is removed
				const auto at = ptr->BannedIp.find_first_of(':');
				if (at != std::string::npos)
					ptr->BannedIp.erase(at);
			}
			else
				continue;
			if (++tok_iter != tokens.end())
				ptr->BannerName = (*tok_iter);
//skip dupe
			if (mode == RELOAD_MODE_TMPFILE)
			{
				const auto i =
					std::find_if(Proxy_Ban_List.begin(), Proxy_Ban_List.end(), [&](const ProxyBanNodePtr p)
				{
					return this->proxy_ban_compare(p, BAN_IP_COMPARE, &ptr->BannedIp);
				});
				if (i != Proxy_Ban_List.end())
					continue;
			}
			Proxy_Ban_List.push_front(ptr);
			if (mode == RELOAD_MODE_TMPFILE)
			{
				disconnectBannedIp(ptr->BannedIp);
				log("(GC) IP: %s banned by reload_proxy_ban.", ptr->BannedIp.c_str());
				imm_log("IP: %s banned by reload_proxy_ban.", ptr->BannedIp.c_str());
			}
		}
		fclose(loaded);
		if (mode == RELOAD_MODE_TMPFILE)
		{
			save_proxy();
			remove(proxy_ban_filename_tmp);
		}
		return true;
	}

	if (mode == RELOAD_MODE_MAIN)
	{
		log("SYSERR: Unable to open proxybanfile");
	}
	return false;
}

bool BanList::save_ip(void)
{
	FILE *loaded;
	if ((loaded = fopen(ban_filename, "w")))
	{
		for (const auto& it : Ban_List)
		{
			fprintf(loaded, "%s %s %ld %s %ld %s\n",
					ban_types[it->BanType], it->BannedIp.c_str(),
					static_cast<long int>(it->BanDate), it->BannerName.c_str(),
					static_cast<long int>(it->UnbanDate), it->BanReason.c_str());
		}
		fclose(loaded);
		return true;
	}
	log("SYSERR: Unable to save banfile");
	return false;
}

bool BanList::save_proxy(void)
{
	FILE *loaded;
	if ((loaded = fopen(proxy_ban_filename, "w")))
	{
		for (const auto& it : Proxy_Ban_List)
		{
			fprintf(loaded, "%s %s\n", it->BannedIp.c_str(), it->BannerName.c_str());
		}
		fclose(loaded);
		return true;
	}
	log("SYSERR: Unable to save proxybanfile");
	return false;
}

void BanList::ShowBannedIp(int sort_mode, CHAR_DATA * ch)
{
	if (Ban_List.empty())
	{
		send_to_char("No sites are banned.\r\n", ch);
		return;
	}

	sort_ip(sort_mode);
	char format[MAX_INPUT_LENGTH], to_unban[MAX_INPUT_LENGTH], buff[MAX_INPUT_LENGTH], *listbuf = 0, *timestr;
	strcpy(format, "%-25.25s  %-8.8s  %-10.10s  %-16.16s %-8.8s\r\n");
	sprintf(buf, format, "Banned Site Name", "Ban Type", "Banned On", "Banned By", "To Unban");

	listbuf = str_add(listbuf, buf);
	sprintf(buf, format,
			"---------------------------------",
			"---------------------------------",
			"---------------------------------",
			"---------------------------------", "---------------------------------");
	listbuf = str_add(listbuf, buf);

	for (const auto& i : Ban_List)
	{
		timestr = asctime(localtime(&(i->BanDate)));
		*(timestr + 10) = 0;
		strcpy(to_unban, timestr);
		sprintf(buff, "%ldh", static_cast<long int>(i->UnbanDate - time(NULL)) / 3600);
		sprintf(buf, format, i->BannedIp.c_str(), ban_types[i->BanType],
				to_unban, i->BannerName.c_str(), buff);
		listbuf = str_add(listbuf, buf);
		strcpy(buf, i->BanReason.c_str());
		strcat(buf, "\r\n");
		listbuf = str_add(listbuf, buf);
	}
	page_string(ch->desc, listbuf, 1);
	free(listbuf);
}

void BanList::ShowBannedIpByMask(int sort_mode, CHAR_DATA * ch, const char *mask)
{
	bool is_find = FALSE;
	if (Ban_List.empty())
	{
		send_to_char("No sites are banned.\r\n", ch);
		return;
	}

	sort_ip(sort_mode);
	char format[MAX_INPUT_LENGTH], to_unban[MAX_INPUT_LENGTH], buff[MAX_INPUT_LENGTH], *listbuf = 0, *timestr;
	strcpy(format, "%-25.25s  %-8.8s  %-10.10s  %-16.16s %-8.8s\r\n");
	sprintf(buf, format, "Banned Site Name", "Ban Type", "Banned On", "Banned By", "To Unban");

	listbuf = str_add(listbuf, buf);
	sprintf(buf, format,
			"---------------------------------",
			"---------------------------------",
			"---------------------------------",
			"---------------------------------", "---------------------------------");
	listbuf = str_add(listbuf, buf);

	for (const auto& i : Ban_List)
	{
		if (strncmp(i->BannedIp.c_str(), mask, strlen(mask)) == 0)
		{
			timestr = asctime(localtime(&(i->BanDate)));
			*(timestr + 10) = 0;
			strcpy(to_unban, timestr);
			sprintf(buff, "%ldh", static_cast<long int>(i->UnbanDate - time(NULL)) / 3600);
			sprintf(buf, format, i->BannedIp.c_str(), ban_types[i->BanType],
					to_unban, i->BannerName.c_str(), buff);
			listbuf = str_add(listbuf, buf);
			strcpy(buf, i->BanReason.c_str());
			strcat(buf, "\r\n");
			listbuf = str_add(listbuf, buf);
			is_find = TRUE;
		};

	}
	if (is_find)
	{
		page_string(ch->desc, listbuf, 1);
	}
	else
	{
		send_to_char("No sites are banned.\r\n", ch);
	}
	free(listbuf);
}

void BanList::ShowBannedProxy(int sort_mode, CHAR_DATA * ch)
{
	if (Proxy_Ban_List.empty())
	{
		send_to_char("No proxies are banned.\r\n", ch);
		return;
	}
	sort_proxy(sort_mode);
	char format[MAX_INPUT_LENGTH];
	strcpy(format, "%-25.25s  %-16.16s\r\n");
	sprintf(buf, format, "Banned Site Name", "Banned By");
	send_to_char(buf, ch);
	sprintf(buf, format, "---------------------------------", "---------------------------------");

	send_to_char(buf, ch);

	for (const auto& i : Proxy_Ban_List)
	{
		sprintf(buf, format, i->BannedIp.c_str(), i->BannerName.c_str());
		send_to_char(buf, ch);
	}
}

int BanList::is_banned(const std::string& ip)
{
	const auto i =
		std::find_if(Proxy_Ban_List.begin(), Proxy_Ban_List.end(), [&](const ProxyBanNodePtr ptr)
	{
		return this->proxy_ban_compare(ptr, BAN_IP_COMPARE, &ip);
	});

	if (i != Proxy_Ban_List.end())
		return BAN_ALL;

	const auto j =
		std::find_if(Ban_List.begin(), Ban_List.end(), [&](const BanNodePtr ptr)
	{
		return this->ban_compare(ptr, BAN_IP_COMPARE, &ip);
	});

	if (j != Ban_List.end())
	{
		if ((*j)->UnbanDate <= time(0))
		{
			sprintf(buf, "Site %s is unbaned (time expired).", (*j)->BannedIp.c_str());
			mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
			Ban_List.erase(j);
			save_ip();
			return BAN_NO;
		}
		return (*j)->BanType;
	}
	return BAN_NO;
}

bool BanList::unban_ip(const std::string& ip, CHAR_DATA * ch)
{
	const auto i =
		std::find_if(Ban_List.begin(), Ban_List.end(), [&](const BanNodePtr ptr)
	{
		return this->ban_compare(ptr, BAN_IP_COMPARE, &ip);
	});

	if (i != Ban_List.end())
	{
////////////////////////////////////////////////////////////////////////
		send_to_char("Site unbanned.\r\n", ch);
		sprintf(buf, "%s removed the %s-player ban on %s.",
				GET_NAME(ch), ban_types[(*i)->BanType], (*i)->BannedIp.c_str());
		mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s removed the %s-player ban on %s.", GET_NAME(ch),
				ban_types[(*i)->BanType], (*i)->BannedIp.c_str());
////////////////////////////////////////////////////////////////////////
		Ban_List.erase(i);
		save_ip();
		return true;
	}
	return false;
}

bool BanList::unban_proxy(const std::string& ip, CHAR_DATA * ch)
{
	const auto i =
		std::find_if(Proxy_Ban_List.begin(), Proxy_Ban_List.end(), [&](const ProxyBanNodePtr ptr)
	{
		return this->proxy_ban_compare(ptr, BAN_IP_COMPARE, &ip);
	});

	if (i != Proxy_Ban_List.end())
	{
////////////////////////////////////////////////////////////////////////
		send_to_char("Proxy unbanned.\r\n", ch);
		sprintf(buf, "%s removed the proxy ban on %s.", GET_NAME(ch), (*i)->BannedIp.c_str());
		mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s removed the proxy ban on %s.", GET_NAME(ch), (*i)->BannedIp.c_str());
////////////////////////////////////////////////////////////////////////
		Proxy_Ban_List.erase(i);
		save_proxy();
		return true;
	}
	return false;
}

bool BanList::unban(const std::string& Ip, CHAR_DATA * ch)
{
	const auto flag1 = unban_ip(Ip, ch);
	const auto flag2 = unban_proxy(Ip, ch);

	return flag1 || flag2;
}

bool BanList::empty_ip()
{
	return Ban_List.empty();
}

bool BanList::empty_proxy()
{
	return Proxy_Ban_List.empty();
}

void BanList::clear_all()
{
	Ban_List.clear();
	Proxy_Ban_List.clear();
}

void BanList::purge_obsolete()
{
	bool purged = false;
	for (std::list < BanNodePtr >::iterator i = Ban_List.begin(); i != Ban_List.end();)
	{
		if ((*i)->UnbanDate <= time(0))
		{
			Ban_List.erase(i++);
			purged = true;
		}
		else
			i++;
	}
	if (purged)
		save_ip();
}

time_t BanList::getBanDate(const std::string& ip)
{
	const auto i =
		std::find_if(Proxy_Ban_List.begin(), Proxy_Ban_List.end(), [&](const ProxyBanNodePtr ptr)
	{
		return this->proxy_ban_compare(ptr, BAN_IP_COMPARE, &ip);
	});

	if (i != Proxy_Ban_List.end())
		return -1;	//infinite ban

	const auto j =
		std::find_if(Ban_List.begin(), Ban_List.end(), [&](const BanNodePtr ptr)
	{
		return this->ban_compare(ptr, BAN_IP_COMPARE, &ip);
	});

	if (j != Ban_List.end())
	{
		return (*j)->UnbanDate;
	}

	return time(0);

}

void BanList::disconnectBannedIp(const std::string& Ip)
{
	for (auto d = descriptor_list; d; d = d->next)
	{
		if (d->host == Ip)
		{
			if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
				return;
			//send_to_char will crash, it char has not been loaded/created yet.
			SEND_TO_Q("Your IP has been banned, disconnecting...\r\n", d);
			if (STATE(d) == CON_PLAYING)
				STATE(d) = CON_DISCONNECT;
			else
				STATE(d) = CON_CLOSE;
		}
	}
	return;
}

//////////////////////////////////////////////////////////////////////////////////////

/**
* ������� �������������� ����������� �� ����� ������������������� ����.
* ���� ����������� ������������ ������ �� ����� ���������� � ���� � � ������
* ������ ����������� � ��������� - ������ ���� �������� �� ������, �.�. ���
* ��������� ������� ������������� ��� �� �������������� ������ � ��� �� �����,
* �� ���� ��� ����� ���� ���������� ����������� ����� ������� register, ��
* �� ��� ������ � �������� �� ��������, �.�. �� ������ ������ ������� �����������
* � ������ ������ �������� ������� �� ��������� � ���� PLR_REGISTERED �� �������. -- Krodo
*/
namespace RegisterSystem
{

typedef std::map<std::string, std::string> EmailListType;
// ������ ������������������ ���
EmailListType email_list;
// ���� ��� �����������/�����
const char* REGISTERED_EMAIL_FILE = LIB_PLRSTUFF"registered-email.lst";
// �.�. ������ ������������ ����� ���� �������� �������, �� ������� ��� ������ � ������ ��������� � add � remove
bool need_save = false;

} // namespace RegisterSystem

// * ���������� ���� � ������ + ������������ ����� PLR_REGISTERED, registered_email �� ������������
void RegisterSystem::add(CHAR_DATA* ch, const char* text, const char* reason)
{
	PLR_FLAGS(ch).set(PLR_REGISTERED);
	if (!text || !reason) return;
	std::stringstream out;
	out << GET_NAME(ch) << " -> " << text << " [" << reason << "]";
	const auto it = email_list.find(GET_EMAIL(ch));
	if (it == email_list.end())
	{
		email_list[GET_EMAIL(ch)] = out.str();
		need_save = true;
	}
}

/**
* �������� ���� �� ������, ������ ����� PLR_REGISTERED � registered_email.
* � ������� ������� �������� ���������� � ������� ������������ �������, ���� �� �� ���� � ������� ��
*/
void RegisterSystem::remove(CHAR_DATA* ch)
{
	PLR_FLAGS(ch).unset(PLR_REGISTERED);
	const auto it = email_list.find(GET_EMAIL(ch));
	if (it != email_list.end())
	{
		email_list.erase(it);
		if (ch->desc)
			ch->desc->registered_email = false;
		need_save = true;
	}
}

/**
* ��������, �������� �� �������� ������������������ �����-���� �������
* \return 0 - ���, 1 - ��
*/
bool RegisterSystem::is_registered(CHAR_DATA* ch)
{
	if (PLR_FLAGGED(ch, PLR_REGISTERED) || (ch->desc && ch->desc->registered_email))
		return true;
	return false;
}

/**
* ����� ������� ���� � ������ ������������������
* \return 0 - �� �����, 1 - �����
*/
bool RegisterSystem::is_registered_email(const std::string& email)
{
	const auto it = email_list.find(email);
	if (it != email_list.end())
		return true;
	return false;
}

/**
* ����� ���������� �� ������������������� ����
* \return ������ ���������� ��� ������ ������, ���� ������ �� �����
*/
const std::string RegisterSystem::show_comment(const std::string& email)
{
	const auto it = email_list.find(email);
	if (it != email_list.end())
		return it->second;
	return "";
}

// * �������� � ������ ������ ���
void RegisterSystem::load()
{
	email_list.clear();

	std::ifstream file(REGISTERED_EMAIL_FILE);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", REGISTERED_EMAIL_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	std::string email, comment;
	while (file >> email)
	{
		ReadEndString(file);
		std::getline(file, comment);
		email_list[email] = comment;
	}
	file.close();
}

// * ���������� ������ ���, ���� ��� ���������
void RegisterSystem::save()
{
	if (!need_save)
		return;
	std::ofstream file(REGISTERED_EMAIL_FILE);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", REGISTERED_EMAIL_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	for (EmailListType::const_iterator it = email_list.begin(); it != email_list.end(); ++it)
		file << it->first << "\n" << it->second << "\n";
	file.close();
	need_save = false;
}

// contains list of banned ip's and proxies + interface
BanList*& ban = GlobalObjects::ban();

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
