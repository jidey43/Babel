#include "ClientBase.h"

ClientBase::ClientBase(ClientStatus status, int id, std::string const & nick, std::string const & login, std::string const & password, std::list<int> contactList)
	: _status(status), _id(id), _nick(nick), _login(login), _password(password), _contactList(contactList)
{
}

ClientBase::ClientBase(std::string const & login)
{
}

ClientBase::~ClientBase()
{
}

int				 ClientBase::getId() const
{
	return _id;
}

std::string ClientBase::getNickname() const
{
	return _nick;
}

std::string const& ClientBase::getLogin() const
{
	return _login;
}

std::string const& ClientBase::getPassword() const
{
	return _password;
}

std::list<int>& ClientBase::getContactList()
{
	return _contactList;
}

ClientStatus ClientBase::getClientStatus() const
{
	return _status;
}

void ClientBase::setClientStatus(ClientStatus status)
{
	_status = status;
}

void ClientBase::setNickname(std::string const & nickname)
{
	_nick = nickname;
}
