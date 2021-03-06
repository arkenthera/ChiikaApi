﻿//----------------------------------------------------------------------------
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
#include "Root/Root.h"
#include "Request/MalManager.h"
#include "Settings/Settings.h"
#include "Request/RequestManager.h"
#include "Database/LocalDataManager.h"
#include "Recognition/MediaPlayerRecognitionManager.h"
#include "Recognition/AnimeRecognition.h"
#include "Seasons/SeasonManager.h"
#include "Logging/FileHelper.h"
#include "Logging/ChiString.h"
#include "Root/ThreadManager.h"
#include <log4cplus/initializer.h>
#include "Logging/LogManager.h"

#include "Common/UtfTools.h"
//----------------------------------------------------------------------------
//Whoops..
MalApiExport ChiikaApi::Root* GlobalInstance = 0;
//----------------------------------------------------------------------------
namespace ChiikaApi
{
	//----------------------------------------------------------------------------
	Root::Root()
	{
		InitializeNULL(m_pSettings);
		InitializeNULL(m_pMalManager);
		InitializeNULL(m_pSeasonManager);
		InitializeNULL(m_pMPRecognition);
		InitializeNULL(m_pRequestManager);
		InitializeNULL(m_pRecognizer);
		InitializeNULL(m_pLocalData);

		GlobalInstance = this;

		log4cplus::Initializer initializer;
	}
	//----------------------------------------------------------------------------
	void Root::Initialize(bool appMode,int log_level,const char* userName,const char* pass,const char* modulePath)
	{
		options.appMode = appMode;
		options.log_level = log_level;
		options.modulePath = modulePath;
		options.userName = userName;
		options.passWord = pass;

		std::string logFile = options.modulePath + "Chiika.log";
		Log::InitLogging(logFile.c_str());

		ChiString version = std::to_string(ChiikaApi_VERSION_MAJOR) + "." + std::to_string(ChiikaApi_VERSION_MINOR) + "." + std::to_string(ChiikaApi_VERSION_PATCH);;
		m_sVersion = version;
		m_sCommitHash = (char*)ChiikaApi_COMMIT_HASH;

		CHIIKALOG_INFO("Initializing settings...");
		m_pSettings = new AppSettings(options.modulePath);


		CHIIKALOG_INFO("Initializing MyAnimelist...");
		m_pMalManager = new MalManager;


		CHIIKALOG_INFO("Initializing Request Manager...");
		m_pRequestManager = new RequestManager;


		CHIIKALOG_INFO("Initializing local Season Manager...");
		m_pSeasonManager = new SeasonManager;


		CHIIKALOG_INFO("Initializing local database...");
		m_pLocalData = new LocalDataManager;




		if(options.userName.size() == 0 || options.passWord.size() == 0)
		{
			options.userName = m_User.GetKeyValue(kUserName);
			options.passWord = m_User.GetKeyValue(kPass);
		}
		else
		{
			m_User.SetKeyValue(kUserName,options.userName);
			m_User.SetKeyValue(kPass,options.passWord);
		}

		CHIIKALOG_INFO(
			"Initializing ChiikaApi" << std::endl <<
			"Options: " << std::endl <<
			"App Mode: " << appMode << std::endl <<
			"Log Level: " << 1 << std::endl <<
			"User: " << userName << std::endl <<
			"ChiikaApi Version " << version << std::endl <<
			"Last HEAD " << m_sCommitHash << std::endl
			);


		/*std::wstring wstr = L"虹色デイズ";
		std::string k = Utf8::wstring_to_utf8(wstr);


		FsPath p = GetAppSettings()->GetDataPath() / "testo.txt";

		pugi::xml_document testo;
		pugi::xml_node root = testo.append_child("root");

		SetXMLValue(root,k.c_str());

		testo.save_file(p.generic_string().c_str());*/
	}
	//----------------------------------------------------------------------------
	Root::~Root()
	{
		CHIIKALOG_INFO("Destroying Chiika Api...");
		Destroy();
	}
	//----------------------------------------------------------------------------
	void Root::Destroy()
	{
		TryDelete(m_pSettings);
		m_pSettings = 0;
		TryDelete(m_pMalManager);
		m_pMalManager = 0;
		TryDelete(m_pRequestManager);
		m_pRequestManager = 0;
		TryDelete(m_pLocalData);
		m_pLocalData = 0;
		TryDelete(m_pSeasonManager);
		m_pSeasonManager = 0;

		m_vSysEventListeners.clear();
		Log::StopLogging();
	}
	//----------------------------------------------------------------------------
	Root* Root::Get()
	{
		return GlobalInstance;
	}
	//----------------------------------------------------------------------------
	ThreadManager* Root::GetThreadManager()
	{
		return m_pThreadManager;
	}
	//----------------------------------------------------------------------------
	RequestManager* Root::GetRequestManager()
	{
		return m_pRequestManager;
	}
	//----------------------------------------------------------------------------
	LocalDataManager* Root::GetLocalDataManager()
	{
		return m_pLocalData;
	}
	//----------------------------------------------------------------------------
	MalManager* Root::GetMyAnimelistManager()
	{
		return m_pMalManager;
	}
	//----------------------------------------------------------------------------
	AppSettings* Root::GetAppSettings()
	{
		return m_pSettings;
	}
	//----------------------------------------------------------------------------
	SeasonManager* Root::GetSeasonManager()
	{
		return m_pSeasonManager;
	}
	//----------------------------------------------------------------------------
	UserInfo& Root::GetUser()
	{
		return m_User;
	}
	//----------------------------------------------------------------------------
	void Root::VerifyUser(RequestListener* listener)
	{
		m_pRequestManager->VerifyUser(listener);
	}
	//----------------------------------------------------------------------------
	void Root::GetMyAnimelist(RequestListener* listener)
	{
		m_pRequestManager->GetMyAnimelist(listener);
	}
	//----------------------------------------------------------------------------
	const AnimeList& Root::GetAnimelist()
	{
		return GetMyAnimelistManager()->GetAnimes();
	}
	//----------------------------------------------------------------------------
	void Root::GetMyMangalist(RequestListener* listener)
	{
		m_pRequestManager->GetMyMangalist(listener);
	}
	//----------------------------------------------------------------------------
	void Root::MalScrape(RequestListener* listener)
	{
		m_pRequestManager->MalScrape(listener);
	}
	//----------------------------------------------------------------------------
	void Root::DownloadImage(RequestListener* listener,const std::string& url)
	{
		//m_pRequestManager->DownloadImage(listener,url);
	}
	//----------------------------------------------------------------------------
	void Root::AnimeScrape(RequestListener* listener,int AnimeId)
	{
		m_pRequestManager->AnimePageScrape(listener,AnimeId);
	}
	//----------------------------------------------------------------------------
	void Root::MalAjax(RequestListener* listener,int AnimeId)
	{
		m_pRequestManager->MalAjax(listener,AnimeId);
	}
	//----------------------------------------------------------------------------
	void Root::SearchAnime(RequestListener* listener,int id,const char* keywords)
	{
		m_pRequestManager->SearchAnime(listener,id,keywords);
	}
	//----------------------------------------------------------------------------
	void Root::RefreshAnimeDetails(RequestListener* listener,int id)
	{
		m_pRequestManager->RefreshAnimeDetails(listener,id);
	}
	//----------------------------------------------------------------------------
	void Root::GetAnimeDetails(RequestListener* listener,int id)
	{
		m_pRequestManager->GetAnimeDetails(listener,id);
	}
	//----------------------------------------------------------------------------
	void Root::UpdateAnime(RequestListener* listener,int AnimeId,int score,int progress,int status)
	{
		m_pRequestManager->UpdateAnime(listener,AnimeId,score,progress,status);
	}
	//----------------------------------------------------------------------------
	void Root::SetUser(UserInfo user)
	{
		KeyList userKeys;
		GetUserInfoKeys(userKeys);

		FOR_(userKeys,j)
		{
			m_User.SetKeyValue(userKeys[j],user.GetKeyValue(userKeys[j]));
		}

		KeyList mangaKeys;
		GetUserInfoMangaKeys(mangaKeys);

		FOR_(mangaKeys,j)
		{
			m_User.Manga.SetKeyValue(mangaKeys[j],user.Manga.GetKeyValue(mangaKeys[j]));
		}

		KeyList animeKeys;
		GetUserInfoAnimeKeys(animeKeys);

		FOR_(animeKeys,j)
		{
			m_User.Anime.SetKeyValue(animeKeys[j],user.Anime.GetKeyValue(animeKeys[j]));
		}

	}
	//----------------------------------------------------------------------------
	void Root::RegisterSystemEventListener(SystemEventListener* sys)
	{
		m_vSysEventListeners.push_back(sys);
	}
	//----------------------------------------------------------------------------
	void Root::UnRegisterSystemEventListener(SystemEventListener* sys)
	{
		std::vector<SystemEventListener*>::iterator listener = std::find(m_vSysEventListeners.begin(),m_vSysEventListeners.end(),sys);

		if(IsValidIt(listener,m_vSysEventListeners))
		{
			m_vSysEventListeners.erase(listener);
		}
	}
	//----------------------------------------------------------------------------
	void Root::FireSystemEvent(SystemEvents evt)
	{
		FOR_(m_vSysEventListeners,i)
		{
			m_vSysEventListeners[i]->OnEvent(evt);
		}
	}
	//----------------------------------------------------------------------------
	void Root::InitDatabase()
	{
		GetLocalDataManager()->Initialize();
	}
	//----------------------------------------------------------------------------
	const UserAnimeList& Root::GetUserAnimelist()
	{
		return GetMyAnimelistManager()->GetAnimeList();
	}
	//----------------------------------------------------------------------------
	const UserMangaList& Root::GetUserMangalist()
	{
		return GetMyAnimelistManager()->GetMangaList();
	}
	//----------------------------------------------------------------------------
	const std::vector<SenpaiItem>& Root::GetSenpaiData()
	{
		return GetSeasonManager()->GetSenpaiData();
	}
	//----------------------------------------------------------------------------
	void Root::PostRequest(RequestInterface* r)
	{
		GetThreadManager()->PostRequest(r);
	}
	//----------------------------------------------------------------------------

	ChiString Root::GetVersion()
	{
		return m_sVersion;
	}
	//----------------------------------------------------------------------------
	ChiString Root::GetHash()
	{
		return m_sCommitHash;
	}
	//----------------------------------------------------------------------------
	void Root::StoreKeys()
	{
		/*RequestApiValueMap.insert(std::make_pair(REQUEST_VERIFY_SUCCESS,strcat(strdup(kRequestVerify),kRequestSuccess)));
		RequestApiValueMap.insert(std::make_pair(REQUEST_VERIFY_ERROR,strcat(strdup(kRequestVerify),kRequestError)));

		RequestApiValueMap.insert(std::make_pair(REQUEST_GETMYANIMELIST_SUCCESS,strcat(strdup(kRequestGetMyAnimelist),kRequestSuccess)));
		RequestApiValueMap.insert(std::make_pair(REQUEST_GETMYANIMELIST_ERROR,strcat(strdup(kRequestGetMyAnimelist),kRequestError)));

		RequestApiValueMap.insert(std::make_pair(REQUEST_GETMYMANGALIST_SUCCESS,strcat(strdup(kRequestGetMyMangalist),kRequestSuccess)));
		RequestApiValueMap.insert(std::make_pair(REQUEST_GETMYMANGALIST_ERROR,strcat(strdup(kRequestGetMyMangalist),kRequestError)));

		RequestApiValueMap.insert(std::make_pair(REQUEST_IMAGEDOWNLOAD_SUCCESS,strcat(strdup(kRequestImageDownload),kRequestSuccess)));
		RequestApiValueMap.insert(std::make_pair(REQUEST_IMAGEDOWNLOAD_ERROR,strcat(strdup(kRequestImageDownload),kRequestError)));

		RequestApiValueMap.insert(std::make_pair(REQUEST_ANIMESCRAPE_SUCCESS,strcat(strdup(kRequestAnimePageScrape),kRequestSuccess)));
		RequestApiValueMap.insert(std::make_pair(REQUEST_ANIMESCRAPE_ERROR,strcat(strdup(kRequestAnimePageScrape),kRequestError)));
		*/

	}
}
