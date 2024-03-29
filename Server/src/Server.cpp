#include "Server.h"
#include <algorithm>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>
#endif
#include <string.h>

Server::Server(std::string const & ip, std::string const & port)
	: _network(getNetworkInstance()), _cPacket(new ClientPacket()), _sPacket(new ServerPacket()), _dataHandler(new ClientDataHandler(std::string("ClientData.xml"), 0, _dataBase)), _ip(ip), _port(port)
{
	_baseID = 0;
	InitNetwork();
	InitFuncMap();
	Start();
}

Server::~Server()
{
}

bool				Server::InitNetwork()
{
	MyConnectionData*	conData;

	_network->initNetwork();
	if ((conData = _network->getAddr(NULL, _port.c_str(), AF_INET, SOCK_STREAM, IPPROTO_TCP, AI_PASSIVE)) == NULL)
		return false;
	if ((_listen = _network->MySocketFunc(conData)) == -1)
		return false;
	if (!_network->MyBindFunc(_listen, conData) || _network->MyListenFunc(_listen) == 0)
		return false;
	return true;
}

void				Server::InitFuncMap()
{
  _funcMap.emplace(AUTH, &Server::Login);
  _funcMap.emplace(NICKNAME, &Server::Nick);
  _funcMap.emplace(GETCLIST, &Server::GetCList);
  _funcMap.emplace(RQ_CALL, &Server::RequestCall);
  _funcMap.emplace(ACPT_CALL, &Server::AcceptCall);
  _funcMap.emplace(REFU_CALL, &Server::RefuseCall);
  _keyVector.push_back(AUTH);
	_keyVector.push_back(NICKNAME);
	_keyVector.push_back(GETCLIST);
	_keyVector.push_back(RQ_CALL);
	_keyVector.push_back(ACPT_CALL);
	_keyVector.push_back(REFU_CALL);
}

void				Server::Start(void)
{
	while (1)
	{
		SetClientFD();
		_network->MySelectFunc(_listen, _readSet, NULL);
		if (_network->CheckFdIsSet(_listen, _readSet))
			StartNewClient();
		CheckClientQueue();
	}
}
 
void				Server::SetClientFD()
{
	_network->ZeroFD(_readSet);
	_network->SetFD(_listen, _readSet);
	for (std::deque<ClientRuntime*>::iterator it = _dataRuntime.begin(); it != _dataRuntime.end(); ++it)
	{
		_network->SetFD((*it)->getSocket(), _readSet);
	}
}

void				Server::CheckClientQueue()
{
	for (std::deque<ClientRuntime*>::iterator it = _dataRuntime.begin(); it != _dataRuntime.end(); it++)
	{
		if (_network->CheckFdIsSet((*it)->getSocket(), _readSet))
		{
			if (_network->rcvMessage((*it)->getSocket(), _cPacket, sizeof(*_cPacket)) <= 0)
			{
				CloseClient(it, (*it));
				std::cout << "Client closed" << std::endl;
				if (it == _dataRuntime.end())
					return;
			}
			else
				CommandParser((*it));
		}
	}
}

void					Server::CloseClient(std::deque<ClientRuntime*>::iterator& it, ClientRuntime* client)
{
	if (_dataHandler->GetBaseClient(client) != NULL)
		_dataHandler->GetBaseClient(client)->setClientStatus(OFFLINE);
	NoticeClientLeft(client);
	_network->CloseConnection(client->getSocket());
	delete (client);
	it = _dataRuntime.erase(std::find(_dataRuntime.begin(), _dataRuntime.end(), client));
}

void				Server::NoticeClientLeft(ClientRuntime* client)
{
	for (std::deque<ClientRuntime*>::iterator it = _dataRuntime.begin(); it != _dataRuntime.end(); ++it)
	{
		if ((*it) != client)
			GetCInfo(_dataHandler->GetBaseClient(client), (*it)->getSocket());
	}
}

void				Server::CommandParser(ClientRuntime* client)
{
	std::cout << "Command parser : " << _cPacket->command << std::endl;
	memset(_sPacket, 0, sizeof(*_sPacket));
	if (CheckCommand(_cPacket->command))
		(this->*_funcMap[_cPacket->command])(client);
	else
	{
		std::cout << "syntax error" << std::endl;
		_sPacket->response = STX_ERR;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
}

MySocket Server::TryAcceptClient()
{
	MySocket			socket;
	struct sockaddr_in	client_sin;
	socklen_t			client_sin_len;

	client_sin_len = sizeof(client_sin);
	if ((socket = accept(_listen, (struct sockaddr*)&client_sin, &client_sin_len)) == -1)
	{
		std::cout << "Error accept :" << std::endl;
		return -1;
	}
	return socket;
}

bool Server::CheckCommand(ClientNetworkCommand command)
{
	for (std::vector<ClientNetworkCommand>::iterator it = _keyVector.begin(); it != _keyVector.end(); ++it)
	{
		if ((*it) == command)
			return true;
	}
	return false;
}

void				Server::StartNewClient()
{
	MySocket		socket = TryAcceptClient();

	if (socket != -1)
		_dataRuntime.push_front(new ClientRuntime(socket));
	std::cout << "Start new client , fd = " << socket << std::endl;
}

/* BUILTIN */

void Server::Login(ClientRuntime* client)
{
	_dataHandler->LoginIsSet(_cPacket->data.Auth.username, client);
	std::cout << "login set" << std::endl;
	if (_dataHandler->IsRightPassword(_cPacket->data.Auth.password, client))
	{
		_sPacket->response = AUTH_OK;
		if (!_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket()))
			std::cout << "error send message" << std::endl;
		GetCInfo(_dataHandler->GetBaseClient(client), client->getSocket());
		NoticeClientLeft(client);
	}
	else
	{
		_sPacket->response = AUTH_KO;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
}

void Server::Nick(ClientRuntime* client)
{
	if (!client->isLoggedIn())
	{
		_sPacket->response = FORBIDDEN;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
	else
	{
		_dataHandler->GetBaseClient(client)->setNickname(_cPacket->data.Nick.nick);
		_sPacket->response = OK;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
}

void Server::GetCInfo(ClientBase* client, MySocket socket)
{
	std::cout << "inside getcinfo" << std::endl;
	memset(_sPacket, 0, sizeof(*_sPacket));
	_sPacket->response = USER_INFO;
	_sPacket->data.UserInfo.id = client->getId();
	strncpy(_sPacket->data.UserInfo.nickname, client->getNickname().c_str(), client->getNickname().size());
	_sPacket->data.UserInfo.status = client->getClientStatus();
	_network->sendMessage(_sPacket, sizeof(*_sPacket), socket);
}

void Server::GetCList(ClientRuntime* client)
{
	if (!client->isLoggedIn())
	{
		_sPacket->response = FORBIDDEN;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
	else
	{
		_sPacket->response = BEG_CTLIST;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
		for (std::deque<ClientBase*>::iterator it = _dataBase.begin(); it != _dataBase.end(); ++it)
		{
			if (_dataHandler->GetBaseClient(client)->getId() != (*it)->getId())
			  GetCInfo((*it), client->getSocket());
		}
		memset(_sPacket, 0, sizeof(*_sPacket));
		_sPacket->response = END_CTLIST;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
}

void Server::RequestCall(ClientRuntime* client)
{
	if (!client->isLoggedIn())
	{
		_sPacket->response = FORBIDDEN;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
	else
	{
		_sPacket->response = INCOM_CALL;
		_sPacket->data.IncomingCall.id = _dataHandler->GetBaseClient(client)->getId();
		strncpy(_sPacket->data.IncomingCall.nickname, _dataHandler->GetBaseClient(client)->getNickname().c_str(), _dataHandler->GetBaseClient(client)->getNickname().size());
		_network->sendMessage(_sPacket, sizeof(*_sPacket), _dataHandler->GetSocketById(_dataRuntime, _cPacket->data.rq_call.id));
	}
}

void Server::AcceptCall(ClientRuntime* client)
{
	std::cout << "id : " << _cPacket->data.acpt_call.id << "   ip : " << _cPacket->data.acpt_call.ip << "   port : " << _cPacket->data.acpt_call.port << std::endl;
	if (!client->isLoggedIn())
	{
		_sPacket->response = FORBIDDEN;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
	else
	{
		_sPacket->response = CALL_RQ_ACPT;
		_sPacket->data.CallRqAccept.id = _cPacket->data.acpt_call.id;
		strncpy(_sPacket->data.CallRqAccept.ip, _cPacket->data.acpt_call.ip, strlen(_cPacket->data.acpt_call.ip));
		strncpy(_sPacket->data.CallRqAccept.port, (char*)_cPacket->data.acpt_call.port, 4);
		_network->sendMessage(_sPacket, sizeof(*_sPacket), _dataHandler->GetSocketById(_dataRuntime, _cPacket->data.acpt_call.id));
	}
}

void Server::RefuseCall(ClientRuntime* client)
{
	if (!client->isLoggedIn())
	{
		_sPacket->response = FORBIDDEN;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), client->getSocket());
	}
	else
	{
		_sPacket->response = CALL_RQ_REFU;
		_sPacket->data.CallRqAccept.id = _cPacket->data.refu_call.id;
		_network->sendMessage(_sPacket, sizeof(*_sPacket), _dataHandler->GetSocketById(_dataRuntime, _cPacket->data.refu_call.id));
	}
}
