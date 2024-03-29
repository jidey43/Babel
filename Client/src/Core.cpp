#include "Core.h"
#include "AudioController.h"

/*	OK = 100,
	KO = 101,
	FORBIDDEN = 102,
	STX_ERR = 103,
	AUTH_OK = 200,
	AUTH_KO = 201,
	USER_INFO = 300,
	BEG_CTLIST = 301,
	END_CTLIST = 302,
	INCOM_CALL = 400,
	CALL_RQ_ACPT = 401,
	CALL_RQ_REFU = 402*/

Core::Core()
{
	_isCommunicate = false;
	_GUImap.emplace(CONNECT, &Core::connectToServer);
	_GUImap.emplace(NICK, &Core::changeNick);
	_GUImap.emplace(CALL, &Core::outcomeCall);
	_NetMap.emplace(USER_INFO, &Core::userInfo);
	_NetMap.emplace(INCOM_CALL, &Core::incomeCall);
	_NetMap.emplace(CALL_RQ_ACPT, &Core::acceptedCall);

}

Core::~Core()
{

}

void Core::start()
{
	_uictrl = new UserInterfaceController;

	QTimer *timer = new QTimer;
	connect(timer, &QTimer::timeout, [=]() { this->events(); });
	timer->start(0);
	
	//_uictrl->insertNewContact("Bobby", 1217);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbette", 1220);
	//_uictrl->insertNewContact("Bobbo", 1221);

	_uictrl->run();
}

/* ACCESS TO THE QT LOOP, THIS CODE WILL BE EXECUTED AT ALL ITERATION OF THE PROG*/
void			Core::events()
{
	treatGuiEvents();
	treatNetEvents();
	if (_isCommunicate)
		calling();
}

void Core::treatGuiEvents()
{
	std::queue<GUIEvent>	*queue = _uictrl->getEvents();
	GUIEvent				current;

	while (!queue->empty())
	{
		current = queue->front();
		queue->pop();
		(this->*_GUImap[current.command])(current);
	}
}

void Core::treatNetEvents()
{
	ServerPacket		*pack;

	pack = _slink.checkMessage();
	if (pack == NULL)
		return;
	if (_NetMap.find(pack->response) != _NetMap.end())
		(this->*_NetMap[pack->response])(pack);
	delete pack;
}

void Core::outcomeCall(GUIEvent event)
{
	_slink.requestCall(event.id);
}

void Core::changeNick(GUIEvent event)
{
	_slink.nickname(event.nickname);
}

void Core::connectToServer(GUIEvent event)
{
	if (_slink.connect(event.ip, event.port) == false)
		_uictrl->connectionError();
	_slink.login(event.username);
	_slink.getContactList();
}

void Core::userInfo(ServerPacket *pack)
{
	std::cout << "USER INFO  "  << pack->data.UserInfo.nickname << "   "  << pack->data.UserInfo.status << std::endl;
	if (pack->data.UserInfo.status == CONNECTED)
		_uictrl->insertNewContact(pack->data.UserInfo.nickname, pack->data.UserInfo.id);
	if (pack->data.UserInfo.status == OFFLINE)
		_uictrl->deleteContact(pack->data.UserInfo.id);
}

void Core::incomeCall(ServerPacket *packet)
{
	bool			response;

	if (_isCommunicate)
		response = false;
	else
		response = _uictrl->callRequest(packet->data.IncomingCall.nickname);

	_slink.sendResponseToCall(response, "127.0.0.1", "4243", packet->data.IncomingCall.id);
	if (response)
	{
		_intercom.Accept(std::string("4243"));
		if ((_isCommunicate = _intercom.TryAccept()) == false)
		{
			std::cout << "not accepted" << std::endl;
			return;
		}
	}
	std::cout << "accepted !\n";
}

void Core::acceptedCall(ServerPacket *pack)
{
	if (_intercom.Connect(std::string(pack->data.CallRqAccept.ip), std::string(pack->data.CallRqAccept.port)) == false)
		return;
	_isCommunicate = true;
	_packetSend = new Sound::Encoded;
	_packetReceived = new Sound::Encoded;
	_audio.startRecord();
	_audio.startPlay();
}

void Core::calling()
{	
	*_packetSend = _audio.SoundEvent();
	memset(_packetSend, 0, sizeof(*_packetSend));
	_intercom.SendData(_packetSend);
	memset(_packetReceived, 0, sizeof(*_packetReceived));
	_intercom.ReceiveData(_packetReceived);
	_audio.player(*_packetReceived);
}

//audio.stopRecord();
//audio.stopPlay();
