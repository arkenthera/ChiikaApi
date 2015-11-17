//----------------------------------------------------------------------------
//Chiika Api
//Copyright (C) 2015  Alperen Gezer
//This program is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along
//with this program; if not, write to the Free Software Foundation, Inc.,
//51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.*/
//----------------------------------------------------------------------------
#include "Stable.h"
#include "RequestInterface.h"
#include "Database\LocalDataManager.h"
#include "Logging\LogManager.h"
//----------------------------------------------------------------------------
struct xml_string_writer : pugi::xml_writer
{
	std::string result;
	virtual void write(const void* data,size_t size)
	{
		result += std::string(static_cast<const char*>(data),size);
	}
};
namespace ChiikaApi
{
	RequestInterface::RequestInterface(LocalDataManager* ldm)
	{
		if(!ldm)
		{
			m_pLocalData = LocalDataManager::GetPtr();
		}
		else
		{
			m_pLocalData = ldm;
		}

	}
	//----------------------------------------------------------------------------
	RequestInterface::~RequestInterface()
	{

	}
	//----------------------------------------------------------------------------
	void RequestInterface::OnSuccess()
	{
		for(size_t i = 0; i < m_vListeners.size(); i++)
		{
			m_vListeners[i]->OnSuccess(this);
		}

		LocalDataManager::Get().SaveAll();
	}
	//----------------------------------------------------------------------------
	void RequestInterface::OnError()
	{
		//
		int error = m_Curl.GetRequestResult();

		if(error == 0)
		{
			LOG(ERROR) << "Wow something went really wrong wtf.";
		}

		if(error & RequestCodes::UNAUTHORIZED)
		{
			LOG(ERROR) << "Authorization error.";
		}

		if(error & RequestCodes::CANT_CONNECT)
		{
			LOG(ERROR) << "Connection couldnt be made.";
		}

		if(error & RequestCodes::CANT_RESOLVE_HOST_OR_PROXY)
		{
			LOG(ERROR) << "Can't resolve host or proxy.Probably there is no internet connection.";
		}

		for(size_t i = 0; i < m_vListeners.size(); i++)
		{
			m_vListeners[i]->OnError(this);
		}

		//ToDo(arkenthera): Implements others sometime.
	}
	//----------------------------------------------------------------------------
	void RequestInterface::AddListener(RequestListener* listener)
	{
		if(listener)
			m_vListeners.push_back(listener);
	}
	//----------------------------------------------------------------------------
	void RequestInterface::RemoveListener(RequestListener* l)
	{
		RequestListener* listener = 0;
		int index;
		for(unsigned int i = 0; i < m_vListeners.size(); i++)
		{
			if(m_vListeners[i] == l)
			{
				listener = l;
				index = i;
			}
		}

		m_vListeners.erase(m_vListeners.begin() + index);
		TryDelete(listener);
	}
	//----------------------------------------------------------------------------
	ChiString RequestInterface::GetAnimeXML(const UserAnimeEntry& anime)
	{
		pugi::xml_document doc;
		pugi::xml_node entry = doc.append_child("entry");
		pugi::xml_node episode = entry.append_child("episode");
		pugi::xml_node status = entry.append_child("status");
		pugi::xml_node score = entry.append_child("score");
		pugi::xml_node downloaded_episodes = entry.append_child("downloaded_episodes");
		pugi::xml_node storage_type = entry.append_child("storage_type");
		pugi::xml_node storage_value = entry.append_child("storage_value");
		pugi::xml_node times_rewatched = entry.append_child("times_rewatched");
		pugi::xml_node rewatch_value = entry.append_child("rewatch_value");
		pugi::xml_node date_start = entry.append_child("date_start");
		pugi::xml_node date_finish = entry.append_child("date_finish");
		pugi::xml_node priority = entry.append_child("priority");
		pugi::xml_node enable_discussion = entry.append_child("enable_discussion");
		pugi::xml_node enable_rewatching = entry.append_child("enable_rewatching");
		pugi::xml_node comments = entry.append_child("comments");
		pugi::xml_node fansub_group = entry.append_child("fansub_group");
		pugi::xml_node tags = entry.append_child("tags");

		pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "UTF-8";


		episode.text().set(anime.WatchedEpisodes);
		status.text().set(anime.Status);
		score.text().set(anime.Score);
		std::stringstream str;
		xml_string_writer writer;

		doc.save(writer);
		return writer.result;
	}

}