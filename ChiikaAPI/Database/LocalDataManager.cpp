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
#include "Database/LocalDataManager.h"
#include "Request/MalManager.h"
#include "Settings/Settings.h"
#include "Seasons/SeasonManager.h"
#include "json/json.h"
#include "Logging/FileHelper.h"
#include "Root/Root.h"
#include "Logging/LogManager.h"

#include "boost/thread.hpp"
#include "boost/bind.hpp"
//----------------------------------------------------------------------------
ChiikaApi::LocalDataManager* gLdm = NULL;
//----------------------------------------------------------------------------
namespace ChiikaApi
{
	LocalDataManager::LocalDataManager()
		: animeListReady(false),
		animeCachedListReady(false),
		mangaListReady(false),
		userInfoReady(false),
		senpaiReady(false)
	{

	}
	void LocalDataManager::Initialize()
	{
		FsPath dataPath = Root::Get()->GetAppSettings()->GetDataPath();

		boost::shared_ptr<boost::asio::io_service> io_service(new boost::asio::io_service);

		boost::shared_ptr< boost::asio::io_service::work > work(
			new boost::asio::io_service::work(*io_service)
			);

		boost::thread_group worker_threads;

		worker_threads.create_thread(boost::bind(&LocalDataManager::WorkerThread,this,io_service));
		worker_threads.create_thread(boost::bind(&LocalDataManager::WorkerThread,this,io_service));
		worker_threads.create_thread(boost::bind(&LocalDataManager::WorkerThread,this,io_service));


		//io_service->post(boost::bind(&LocalDataManager::LoadSenpaiData,this));

		io_service->post(boost::bind(&LocalDataManager::LoadCachedAnimelist,this));
		io_service->post(boost::bind(&LocalDataManager::LoadSenpaiData,this));
		io_service->post(boost::bind(&LocalDataManager::LoadAnimeList,this));
		io_service->post(boost::bind(&LocalDataManager::LoadMangaList,this));
		io_service->post(boost::bind(&LocalDataManager::LoadUserInfo,this));


		work.reset();

		//worker_threads.join_all();


	}
	void LocalDataManager::CheckIfFsLoaded()
	{
		if(animeListReady && mangaListReady && userInfoReady && animeCachedListReady && senpaiReady)
		{
			//It's ready
			CHIIKALOG_DEBUG("File system is ready.");

			Root::Get()->FireSystemEvent(SystemEvents::CHIIKA_API_FS_READY);
		}
	}
	void LocalDataManager::WorkerThread(boost::shared_ptr< boost::asio::io_service > io_service)
	{
		CHIIKALOG_DEBUG("IO thread running (" << boost::this_thread::get_id() << ")");

		io_service->run();

		CHIIKALOG_DEBUG("IO thread exiting (" << boost::this_thread::get_id() << ")");
	}

	LocalDataManager::~LocalDataManager()
	{

	}
	//----------------------------------------------------------------------------
	void LocalDataManager::SaveAnimeList()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetAnimelistPath();
		FileWriter file(dataFile);


		if(file.Open())
		{
			pugi::xml_document doc;

			XmlNode root = doc.append_child(kChiika);
			XmlNode  MAL = root.append_child(kMyAnimeList);

			ChiikaApi::UserAnimeList list = Root::Get()->GetMyAnimelistManager()->GetAnimeList();
			ChiikaApi::UserAnimeList::iterator It;
			for(It = list.begin(); It != list.end(); ++It)
			{
				UserAnimeEntry Anime = It->second;


				XmlNode  anime = MAL.append_child(kAnime);

				KeyList uaeList;
				GetUserAnimeEntryKeys(uaeList);

				for(size_t i = 0; i < uaeList.size(); i++)
				{
					XmlNode node = anime.append_child(ToStd(uaeList[i]));
					SetXMLValue(node,ToStd(Anime.GetKeyValue(uaeList[i])));
				}
			}
			doc.save_file(dataFile.c_str());
			file.Close();
		}
		else
		{

		}
	}
	//----------------------------------------------------------------------------
	void LocalDataManager::SaveCachedAnimeList()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetCachedAnimelistPath();
		FileWriter file(dataFile);


		if(file.Open())
		{
			pugi::xml_document doc;
			XmlNode  root = doc.append_child(kChiika);

			ChiikaApi::AnimeList list = Root::Get()->GetMyAnimelistManager()->GetAnimes();

			KeyList animeKeys;
			GetAnimeKeys(animeKeys);

			KeyList miscKeys;
			GetAnimeMiscKeys(miscKeys);

			ChiikaApi::AnimeList::iterator It = list.begin();
			for(It;It != list.end();++It)
			{
				Anime anime = It->second;
				XmlNode animeNode = root.append_child(kAnime);

				FOR_(animeKeys,i)
				{
					XmlNode node = animeNode.append_child(ToStd(animeKeys[i]));
					SetXMLValue(node,ToStd(anime.GetKeyValue(animeKeys[i])));
				}

				XmlNode misc = animeNode.append_child(kMisc);

				AnimeMisc animeMisc = anime.Misc;

				FOR_(miscKeys,k)
				{
					if(miscKeys[k] == "japanese")
					{
						if(animeMisc.GetKeyValue(miscKeys[k]) != "japanese")
						{
							int x = 0;
						}
					}
					XmlNode node = misc.append_child(ToStd(miscKeys[k]));
					SetXMLValue(node,ToStd(animeMisc.GetKeyValue(miscKeys[k])));
				}
				KeyList studiokeys ={kStudioId,kStudioName};
				KeyList characterKeys ={kCharacterId,kCharacterName,kCharacter,kCharacterImage,kCharacterVa,kCharacterVaId};
				KeyList genreKeys ={kGenre};

				XmlNode genresNode = misc.append_child(kGenres);
				XmlNode charactersNode = misc.append_child(kCharacters);
				XmlNode studiosNode = misc.append_child(kStudios);

				FOR_(animeMisc.Studios,b)
				{
					DictionaryBase s = animeMisc.Studios[b];
					XmlNode innerStudio = studiosNode.append_child(kStudio);

					FOR_(studiokeys,a)
					{
						XmlNode studioNode = innerStudio.append_child(ToStd(studiokeys[a]));
						SetXMLValue(studioNode,ToStd(s.GetKeyValue(studiokeys[a])));
					}
				}

				FOR_(animeMisc.Characters,b)
				{
					DictionaryBase c = animeMisc.Characters[b];
					XmlNode innerCh = charactersNode.append_child(kCharacter);

					FOR_(characterKeys,a)
					{
						XmlNode characterNode = innerCh.append_child(ToStd(characterKeys[a]));
						SetXMLValue(characterNode,ToStd(c.GetKeyValue(characterKeys[a])));
					}
				}

				FOR_(animeMisc.Genres,b)
				{
					DictionaryBase g = animeMisc.Genres[b];
					FOR_(genreKeys,a)
					{
						XmlNode genreNode = genresNode.append_child(ToStd(genreKeys[a]));
						SetXMLValue(genreNode,ToStd(g.GetKeyValue(genreKeys[a])));
					}
				}
			}


			doc.save_file(dataFile.c_str());
			file.Close();
		}
		else
		{

		}
	}
	//----------------------------------------------------------------------------
	void LocalDataManager::LoadAnimeList()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetAnimelistPath();
		FileReader file(dataFile);


		if(file.Open())
		{
			pugi::xml_document doc;
			ChiString fileData = file.Read();
			doc.load(fileData.c_str());
			file.Close();

			XmlNode  root = doc.child(kChiika);
			XmlNode  myanimelist = root.child(kMyAnimeList);

			UserAnimeList list;
			for(XmlNode anime = myanimelist.child(kAnime); anime; anime = anime.next_sibling())
			{
				UserAnimeEntry info;
				for(XmlNode animeChild = anime.first_child(); animeChild; animeChild = animeChild.next_sibling())
				{
					const char* name = animeChild.name();
					const char* val = animeChild.text().get();

					info.SetKeyValue(name,val);
				}

				list.insert(UserAnimeList::value_type(((info.GetKeyValue(kSeriesAnimedbId))),info));
			}

			{
				SCOPED_LOCK;
				Root::Get()->GetMyAnimelistManager()->AddAnimeList(list);
				file.Close();

				animeListReady = true;
				CheckIfFsLoaded();
			}

		}
		else
		{

		}
	}
	//----------------------------------------------------------------------------
	void LocalDataManager::SaveMangaList()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetMangalistPath();
		FileWriter file(dataFile);

		if(file.Open())
		{
			pugi::xml_document doc;

			XmlNode root = doc.append_child(kChiika);
			XmlNode  MAL = root.append_child(kMangaList);

			ChiikaApi::UserMangaList list = Root::Get()->GetMyAnimelistManager()->GetMangaList();
			ChiikaApi::UserMangaList::iterator It;
			for(It = list.begin(); It != list.end(); ++It)
			{
				UserMangaEntry Manga = It->second;


				XmlNode  manga = MAL.append_child(kManga);
				KeyList keys;
				GetMangaKeys(keys);


				for(size_t i = 0; i < keys.size(); i++)
				{
					XmlNode node = manga.append_child(ToStd(keys[i]));
					SetXMLValue(node,ToStd(Manga.GetKeyValue(keys[i])));
				}

				KeyList uaeList;
				GetUserMangaEntryKeys(uaeList);

				for(size_t i = 0; i < uaeList.size(); i++)
				{
					XmlNode node = manga.append_child(ToStd(uaeList[i]));
					SetXMLValue(node,ToStd(Manga.GetKeyValue(uaeList[i])));
				}
			}
			doc.save_file(dataFile.c_str());
			file.Close();
		}
		else
		{

		}
	}
	//----------------------------------------------------------------------------
	void LocalDataManager::LoadMangaList()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetMangalistPath();
		FileReader file(dataFile);


		if(file.Open())
		{
			pugi::xml_document doc;
			ChiString fileData = file.Read();
			doc.load(fileData.c_str());
			file.Close();

			XmlNode  root = doc.child(kChiika);
			XmlNode  mymangalist = root.child(kMangaList);

			ChiikaApi::MangaList mangaList;
			ChiikaApi::UserMangaList list;
			for(XmlNode manga = mymangalist.child(kManga); manga; manga = manga.next_sibling())
			{
				Manga nManga;
				UserMangaEntry info;

				for(XmlNode mangaChild = manga.first_child(); mangaChild; mangaChild = mangaChild.next_sibling())
				{
					const char* name = mangaChild.name();
					const char* val = mangaChild.text().get();

					nManga.SetKeyValue(name,val);
					info.SetKeyValue(name,val);
				}
				mangaList.insert(ChiikaApi::MangaList::value_type(nManga.GetKeyValue(kSeriesAnimedbId),nManga));
				
				list.insert(UserMangaList::value_type(nManga.GetKeyValue(kSeriesMangadbId),info));
			}


			{
				SCOPED_LOCK;
				Root::Get()->GetMyAnimelistManager()->AddMangaList(list);
				Root::Get()->GetMyAnimelistManager()->AddMangaList(mangaList);

				mangaListReady = true;
				CheckIfFsLoaded();
			}

			file.Close();
		}
		else
		{

		}
	}
	//----------------------------------------------------------------------------
	void LocalDataManager::SetUserNamePass(ChiString u,ChiString p)
	{

	}
	//----------------------------------------------------------------------------
	void LocalDataManager::SaveUserInfo()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetUserInfoPath();
		FileWriter file(dataFile);

		if(file.Open())
		{
			pugi::xml_document doc;
			XmlNode  root = doc.append_child(kChiika);
			XmlNode  UserInfo = root.append_child(kUserInfo);
			XmlNode	 AnimeInfo = UserInfo.append_child(kAnime);
			XmlNode	 MangaInfo = UserInfo.append_child(kManga);

			ChiikaApi::UserInfo ui = Root::Get()->GetUser();

			KeyList keys;
			GetUserInfoKeys(keys);

			FOR_(keys,i)
			{
				XmlNode node = UserInfo.append_child(ToStd(keys[i]));
				SetXMLValue(node,ToStd(ui.GetKeyValue(keys[i])));
			}

			KeyList animeKeys;
			GetUserInfoAnimeKeys(animeKeys);


			FOR_(animeKeys,i)
			{
				XmlNode node = AnimeInfo.append_child(ToStd(animeKeys[i]));
				SetXMLValue(node,ToStd(ui.Anime.GetKeyValue(animeKeys[i])));
			}

			KeyList mangaKeys;
			GetUserInfoMangaKeys(mangaKeys);


			FOR_(mangaKeys,i)
			{
				XmlNode node = MangaInfo.append_child(ToStd(mangaKeys[i]));
				std::string val = ui.Manga.GetKeyValue(mangaKeys[i]);
				SetXMLValue(node,ToStd(val));
			}

			doc.save_file(dataFile.c_str());
			file.Close();
		}
		else
		{

		}
	}
	//----------------------------------------------------------------------------
	void LocalDataManager::LoadUserInfo()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetUserInfoPath();
		FileReader file(dataFile);

		if(file.Open())
		{
			pugi::xml_document doc;
			ChiString fileData = file.Read();
			doc.load(fileData.c_str());


			XmlNode  root = doc.child(kChiika);
			XmlNode  info = root.child(kUserInfo);
			XmlNode	 anime = info.child(kAnime);
			XmlNode	 manga = info.child(kManga);

			UserInfo uiTemp;

			for(XmlNode infoChild = info.first_child(); infoChild; infoChild = infoChild.next_sibling())
			{
				const char* name = infoChild.name();
				const char* val = infoChild.text().get();

				if(strcmp(name,kAnime) == 0 || strcmp(name,kManga) == 0)
					continue;

				uiTemp.SetKeyValue(name,val);
			}

			for(XmlNode animeChild = anime.first_child(); animeChild; animeChild = animeChild.next_sibling())
			{
				const char* name = animeChild.name();
				const char* val = animeChild.text().get();

				uiTemp.Anime.SetKeyValue(name,val);
			}

			for(XmlNode mangaChild = manga.first_child(); mangaChild; mangaChild = mangaChild.next_sibling())
			{
				const char* name = mangaChild.name();
				const char* val = mangaChild.text().get();

				uiTemp.Manga.SetKeyValue(name,val);
			}


			{
				SCOPED_LOCK;
				Root::Get()->SetUser(uiTemp);

				userInfoReady = true;
				CheckIfFsLoaded();
			}
			file.Close();

		}
		else
		{

		}
	}
	//----------------------------------------------------------------------------
	void LocalDataManager::SaveSenpaiData()
	{

	}
	//----------------------------------------------------------------------------
	void LocalDataManager::LoadSenpaiData()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetSenpaiMoePath();
		FileReader file(dataFile);


		if(file.Open())
		{
			Json::Value root;
			Json::Reader reader;
			ChiString fileData = file.Read();

			bool b = reader.parse(fileData,root);

			if(b)
			{
				//Root
				const Json::Value meta = root["meta"];
				ChiString season = (meta["season"].asString());
				ChiString start = (meta["start"].asString());

				const Json::Value tz = meta["tz"];
				Json::Value::const_iterator It = tz.begin();

				TimezoneMap list;
				for(It; It != tz.end(); It++)
				{
					Json::Value inner = *It;

					Timezone tz;
					tz.Name = (inner["abbr"].asString());
					tz.TimezoneIdentifier = (It.name());
					tz.Offset = (inner["offset"].asString());

					list.insert(TimezoneMap::value_type(tz.TimezoneIdentifier,tz));
				}

				//Items
				const Json::Value items = root["items"];
				Json::Value::const_iterator itemsIt = items.begin();
				SenpaiData sd;

				for(itemsIt; itemsIt != items.end(); itemsIt++)
				{
					Json::Value v = *itemsIt;

					SenpaiItem si;
					si.Name = JsToQ(v["name"]);
					si.MalID = atoi(JsToQ(v["MALID"]).c_str());
					si.IsSequel = (v["isSequel"].asBool());
					si.Simulcast = JsToQ(v["simulcast"]);
					si.AirdateStr = JsToQ(v["airdate"]);
					si.Fansub = JsToQ(v["fansub"]);
					si.SimulcastDelay = v["simulcast_delay"].asInt();
					si.Type = JsToQ(v["type"]);
					si.MissingAirdate = v["missingAirdate"].asBool();
					si.MissingAirtime = v["missingAirtime"].asBool();
					si.AirDateOriginal = JsToQ(v["airdate_orig"]);


					const Json::Value airdates = v["airdates"];

					Json::Value::const_iterator adIt = airdates.begin();
					Map<ChiString,Airdate>::type airdateList;

					ForEachOnStd(adIt,airdates)
					{
						Json::Value t = *adIt;

						Airdate ad;

						ChiString timezoneValue = (adIt.name());

						TimezoneMap::iterator timezoneIt = list.find(timezoneValue);

						if(timezoneIt != list.end())
							ad.TimeZone = list.find(timezoneValue)->second;
						ad.RdDate = JsToQ(t["rd_date"]);
						ad.RdTime = JsToQ(t["rd_time"]);
						ad.Weekday = t["weekday"].asInt();
						ad.RdWeekday = JsToQ(t["rd_weekday"]);

						airdateList.insert(std::make_pair(ad.TimeZone.TimezoneIdentifier,ad));
					}
					si.Airdates = airdateList;
					sd.push_back(si);
				}

				{
					SCOPED_LOCK;
					Root::Get()->GetSeasonManager()->SetTimezones(list);
					Root::Get()->GetSeasonManager()->SetSenpaiData(sd);
					senpaiReady = true;
					CheckIfFsLoaded();
				}

			}
			else
			{
			}

			file.Close();
		}
	}
	//----------------------------------------------------------------------------
	void LocalDataManager::LoadCachedAnimelist()
	{
		FsPath dataFile = Root::Get()->GetAppSettings()->GetCachedAnimelistPath();
		FileReader file(dataFile);


		if(file.Open())
		{
			pugi::xml_document doc;
			ChiString fileData = file.Read();
			doc.load(fileData.c_str());

			XmlNode chiika = doc.child(kChiika);

			ChiikaApi::AnimeList list;
			for(XmlNode chiikaChild = chiika.first_child(); chiikaChild; chiikaChild = chiikaChild.next_sibling())
			{
				Anime anime;
				const char* name = chiikaChild.name();
				const char* val = chiikaChild.text().get();

				for(XmlNode animeChild = chiikaChild.first_child(); animeChild; animeChild = animeChild.next_sibling())
				{
					const char* animeChildName = animeChild.name();
					const char* animeChildVal= animeChild.text().get();


					if(strcmp(animeChildName,kMisc) != 0)
					{
						anime.SetKeyValue(animeChildName,animeChildVal);
					}
					else
					{
						for(XmlNode miscChild = animeChild.first_child();miscChild;miscChild = miscChild.next_sibling())
						{
							const char* miscChildName = miscChild.name();
							const char* miscChildVal  = miscChild.text().get();

							if(strcmp(miscChildName,kGenres) == 0 || strcmp(miscChildName,kCharacters) == 0 || strcmp(miscChildName,kStudios) == 0)
							{
								if(strcmp(miscChildName,kGenres) == 0)
								{
									for(XmlNode genreChild = miscChild.first_child(); genreChild; genreChild = genreChild.next_sibling())
									{
										Genre g;
										const char* genreChildName = genreChild.name();
										const char* genreChildVal  = genreChild.text().get();
										g.SetKeyValue(kGenre,genreChildVal);
										anime.Misc.Genres.push_back(g);
									}
								}
								if(strcmp(miscChildName,kCharacters) == 0)
								{
									for(XmlNode characterChild = miscChild.first_child(); characterChild; characterChild = characterChild.next_sibling())
									{
										Character c;

										for(XmlNode ch = characterChild.first_child(); ch; ch = ch.next_sibling())
										{
											const char* characterChildName = ch.name();
											const char* characterChildVal  = ch.text().get();


											c.SetKeyValue(characterChildName,characterChildVal);
										}
										anime.Misc.Characters.push_back(c);
									}
								}
								if(strcmp(miscChildName,kStudios) == 0)
								{
									for(XmlNode studioChild = miscChild.first_child(); studioChild; studioChild = studioChild.next_sibling())
									{
										Studio s;

										for(XmlNode ch = studioChild.first_child(); ch; ch = ch.next_sibling())
										{
											const char* studioChildName = ch.name();
											const char* studioChildVal  = ch.text().get();


											s.SetKeyValue(studioChildName,studioChildVal);
										}
										anime.Misc.Studios.push_back(s);
									}
								}
							}
							else
							{
								anime.Misc.SetKeyValue(miscChildName,miscChildVal);
							}
						}
					}
				}
				list.insert(ChiikaApi::AnimeList::value_type(anime.GetKeyValue(kSeriesAnimedbId),anime));
			}
			Root::Get()->GetMyAnimelistManager()->AddAnimeList(list);
			file.Close();
			animeCachedListReady = true;
			CheckIfFsLoaded();
		}

	}
	//----------------------------------------------------------------------------
	void LocalDataManager::SaveAll()
	{

	}
	//----------------------------------------------------------------------------
	LocalDataManager* LocalDataManager::Get()
	{
		return gLdm;
	}
	//----------------------------------------------------------------------------
}
