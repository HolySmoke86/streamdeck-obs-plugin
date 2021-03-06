#include "streamdeckplugin_module.h"
#include "infodialog.h"
#include "actionhelp.h"
#include <QMainWindow>
#include <QAction>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QtNetwork>
#include <QLocalSocket>

#include "JSONUtils.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

#define OBS_PORT 28195  

ActionHelp *actionHelpPtr = NULL;

QTcpServer *tcpServer = nullptr;

// ----------------------------------------------------------------------------
// OBS Module Callback
// ----------------------------------------------------------------------------
void ItemMuted(void* ptr, calldata_t* calldata)
{
	Q_UNUSED(ptr);

	//We could use the data, when introducing a new event, which may take them as parameter
	Q_UNUSED(calldata);
	
	//obs_source_t* source = (obs_source_t*)calldata_ptr(calldata, "source");
	//std::string name = obs_source_get_name(source);
	//std::string id = obs_source_get_id(source);
	//bool muted = calldata_bool(calldata, "muted");

	//    qDebug() << __FUNCTION__ << muted << QString::fromStdString(name) << type << QString::fromStdString(id) << QString::fromStdString(displayName);

	json eventJson;
	eventJson["jsonrpc"] = "2.0";
	json result = json::object();
	result["_type"] = "EVENT";
	eventJson["id"] = nullptr;

	result["resourceId"] = "SourcesService.sourceUpdated";
	eventJson["result"] = result;

	std::string str = eventJson.dump() + "\n";
	actionHelpPtr->WriteToSocket(str);
}

void UpdateSource()
{
    qDebug() << __FUNCTION__;

    QString errStr;
    QList<SourceInfo> list;

    actionHelpPtr->UpdateSourcesList(list);

    for (int i=0; i<list.count(); i++)
    {
        SourceInfo srcInfo = list.at(i);
        signal_handler_t* signalHandler = obs_source_get_signal_handler(srcInfo.source);
        signal_handler_connect(signalHandler, "mute", ItemMuted, nullptr);
	}
}

void ItemVisible(void* ptr, calldata_t* calldata)
{
	Q_UNUSED(ptr);

	//We could use the data, when introducing a new event, which may take them as parameter
	Q_UNUSED(calldata);

	//obs_sceneitem_t* sceneItem = (obs_sceneitem_t*)calldata_ptr(calldata, "item");
	//int64_t sceneItemId = obs_sceneitem_get_id(sceneItem);
	//obs_source_t* source = obs_sceneitem_get_source(sceneItem);
	//std::string name = obs_source_get_name(source);
	//std::string id = obs_source_get_id(source);
	//bool visible = calldata_bool(calldata, "visible");

	json eventJson;
	eventJson["jsonrpc"] = "2.0";
	json result = json::object();
	result["_type"] = "EVENT";
	eventJson["id"] = nullptr;

	result["resourceId"] = "ScenesService.itemUpdated";
	eventJson["result"] = result;

	std::string str = eventJson.dump() + "\n";
	actionHelpPtr->WriteToSocket(str);
}

void ItemAdd(void* ptr, calldata_t* calldata)
{
	Q_UNUSED(ptr);
	Q_UNUSED(calldata);

    // send to SD
    if (actionHelpPtr && actionHelpPtr->GetIsRespondingCollectionsSchemaFlag())
    {
		json eventJson;
		eventJson["jsonrpc"] = "2.0";
		json result = json::object();
		result["_type"] = "EVENT";
		eventJson["id"] = nullptr;

		result["resourceId"] = "ScenesService.itemAdded";
		eventJson["result"] = result;

		std::string str = eventJson.dump() + "\n";
		actionHelpPtr->WriteToSocket(str);
    }
}

void ItemRemove(void* ptr, calldata_t* calldata)
{
	Q_UNUSED(ptr);
	Q_UNUSED(calldata);

	if (actionHelpPtr && actionHelpPtr->GetIsRespondingCollectionsSchemaFlag())
	{
		json eventJson;
		eventJson["jsonrpc"] = "2.0";
		json result = json::object();
		result["_type"] = "EVENT";
		eventJson["id"] = nullptr;

		result["resourceId"] = "ScenesService.itemRemoved";
		eventJson["result"] = result;

		std::string str = eventJson.dump() + "\n";
		actionHelpPtr->WriteToSocket(str);
	}
}

void UpdateScenes()
{
    qDebug() << __FUNCTION__;

    // Enumerate scenes for current scene collection
    struct obs_frontend_source_list scenes = { };
    obs_frontend_get_scenes(&scenes);

    // Cycle through scenes
    for (size_t i = 0; i < scenes.sources.num; i++)
    {
        // Connect signal handler
        signal_handler_t* signalHandler = obs_source_get_signal_handler(scenes.sources.array[i]);
        signal_handler_connect(signalHandler, "item_visible", ItemVisible, nullptr);
        signal_handler_connect(signalHandler, "item_add", ItemAdd, nullptr);
        signal_handler_connect(signalHandler, "item_remove", ItemRemove, nullptr);
    }

    // Cleanup
    obs_frontend_source_list_free(&scenes);
}

void OBSEvent(enum obs_frontend_event event, void* data)
{
    Q_UNUSED(data)

    qDebug() << __FUNCTION__ << QThread::currentThread();

	json eventJson;
	eventJson["jsonrpc"] = "2.0";
	json result = json::object();
	result["_type"] = "EVENT";
	eventJson["id"] = nullptr;

	switch (event)
	{
		case OBS_FRONTEND_EVENT_STREAMING_STARTING:
		{
			qDebug() << "OBS_FRONTEND_EVENT_STREAMING_STARTING";

			if (actionHelpPtr->GetIsRespondingStreamingFlag())
			{
				result["data"] = "starting";
				result["resourceId"] = "StreamingService.streamingStatusChange";

				eventJson["result"] = result;

				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}
		}
		break;
		
		case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_STREAMING_STARTED";

			if (actionHelpPtr->GetIsRespondingStreamingFlag())
			{
				result["data"] = "live";
				result["resourceId"] = "StreamingService.streamingStatusChange";

				eventJson["result"] = result;

				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}
		}
		break;
			
		case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		{
			qDebug() << "OBS_FRONTEND_EVENT_STREAMING_STOPPING";

			if (actionHelpPtr->GetIsRespondingStreamingFlag())
			{
				result["data"] = "stopping";
				result["resourceId"] = "StreamingService.streamingStatusChange";

				eventJson["result"] = result;

				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}
		}
		break;
		
		case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_STREAMING_STOPPED";

			if (actionHelpPtr->GetIsRespondingStreamingFlag())
			{
				result["data"] = "offline";
				result["resourceId"] = "StreamingService.streamingStatusChange";

				eventJson["result"] = result;

				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}
		}
		break;
		
		case OBS_FRONTEND_EVENT_RECORDING_STARTING:
		{
			if (actionHelpPtr->GetIsRespondingStreamingFlag())
			{
				result["data"] = "starting";
				result["resourceId"] = "StreamingService.recordingStatusChange";

				eventJson["result"] = result;

				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}

			qDebug() << "OBS_FRONTEND_EVENT_RECORDING_STARTING";
		}
		break;
		
		case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_RECORDING_STARTED";

			if (actionHelpPtr->GetIsRespondingStreamingFlag())
			{
				result["data"] = "recording";
				result["resourceId"] = "StreamingService.recordingStatusChange";

				eventJson["result"] = result;

				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}
		}
		break;
		
		case OBS_FRONTEND_EVENT_RECORDING_STOPPING:
		{
			if (actionHelpPtr->GetIsRespondingStreamingFlag())
			{
				result["data"] = "stopping";
				result["resourceId"] = "StreamingService.recordingStatusChange";

				eventJson["result"] = result;

				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}
			qDebug() << "OBS_FRONTEND_EVENT_RECORDING_STOPPING";
		}
		break;
		
		case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_RECORDING_STOPPED";

			if (actionHelpPtr->GetIsRespondingStreamingFlag())
			{
				result["data"] = "offline";
				result["resourceId"] = "StreamingService.recordingStatusChange";

				eventJson["result"] = result;

				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}
		}
		break;
		
		case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_SCENE_CHANGED";       

			if (actionHelpPtr->GetIsRespondingCollectionsSchemaFlag())
			{
				QMetaObject::invokeMethod(actionHelpPtr, "NotifySceneSwitched");
			}
		}
		break;
		
		case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED";

			UpdateScenes();

			//"sceneAdded"-signal used as a generic changed signal. SD will request all collections then
			result["resourceId"] = "ScenesService.sceneAdded";
			eventJson["result"] = result;

			if (actionHelpPtr->GetIsRespondingCollectionsSchemaFlag())
			{
				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}
		}
		break;
		
		case OBS_FRONTEND_EVENT_TRANSITION_CHANGED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_TRANSITION_CHANGED";
		}
		break;
		
		case OBS_FRONTEND_EVENT_TRANSITION_STOPPED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_TRANSITION_STOPPED";
		}
		break;
		
		case OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED";
		}
		break;
		
		case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		{				
			qDebug() << "OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED";

			UpdateSource();
			UpdateScenes();

			if (actionHelpPtr->GetIsRespondingCollectionsSchemaFlag())
			{
				QMetaObject::invokeMethod(actionHelpPtr, "NotifyCollectionChanged");
			}
		}
		break;
		
		case OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED";

			//Added signal used as "generic" changed signal. SD will request all collections then
			result["resourceId"] = "SceneCollectionsService.collectionAdded";
			eventJson["result"] = result;

			if (actionHelpPtr->GetIsRespondingCollectionsSchemaFlag())
			{
				std::string str = eventJson.dump() + "\n";
				actionHelpPtr->WriteToSocket(str);
			}

		}
		break;
		
		case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_PROFILE_CHANGED";
		}
		break;
		
		case OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED:
		{
			qDebug() << "OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED";
		}
		break;
		
		case OBS_FRONTEND_EVENT_EXIT:
		{
			qDebug() << "OBS_FRONTEND_EVENT_EXIT";
		}
		break;
		
		default:
		{
			qDebug() << "Unsupported event " << event;
		}
		break;
	}

    if (event == OBS_FRONTEND_EVENT_EXIT)
        FreeStreamDeckPlugin();

    // needed to reset the transition, but not working properly now (only some transitions work)
    // if (event == OBS_FRONTEND_EVENT_TRANSITION_STOPPED){
    //	SwitcherData *test = (SwitcherData*)switcher;
    //	test->transitionWaitMutex2.lock();
    //	this_thread::sleep_for(chrono::milliseconds(100)); //seems necesssary
    // since the transition is not always done
    //	test->transitionCv.notify_one();
    //	test->transitionWaitMutex2.unlock();
    //
    //}
}

void SaveCallback(obs_data_t* save_data, bool saving, void*)
{
	Q_UNUSED(save_data)

    qDebug() << __FUNCTION__ << QThread::currentThread() << "saving: " << saving;

    static bool first = true;
    if (first)
    {
        // connect source changing state signal.
        first = false;

        UpdateSource();
        UpdateScenes();

        QTimer::singleShot(1500, actionHelpPtr, SLOT(reqCurrentCollectionAndSceneName()));
    }
    else if (saving)
    {
        UpdateSource();

		if (actionHelpPtr-> GetIsRespondingCollectionsSchemaFlag())
		{
			json eventJson;
			eventJson["jsonrpc"] = "2.0";
			json result = json::object();
			result["_type"] = "EVENT";
			eventJson["id"] = nullptr;

			result["resourceId"] = "SourcesService.sourceUpdated";

			std::string str = eventJson.dump() + "\n";
			actionHelpPtr->WriteToSocket(str);
		}
    }
}

void FreeStreamDeckPlugin()
{
    if (actionHelpPtr)
    {
        delete actionHelpPtr;
        actionHelpPtr = NULL;
    }

	if (tcpServer)
	{
	    delete tcpServer;
		tcpServer = NULL;
	}
}

void InitStreamDeckPlugin()
{
#if 1
    // setup tools menu action for show pluging info
    QAction* action = (QAction*)obs_frontend_add_tools_menu_qaction(obs_module_text("Elgato Stream Deck Plugin"));

    auto cb = []()
    {
        obs_frontend_push_ui_translation(obs_module_get_string);
        QMainWindow* window = (QMainWindow*)obs_frontend_get_main_window();
        InfoDialog dialog(window);
        dialog.exec();

        obs_frontend_pop_ui_translation();
    };
    action->connect(action, &QAction::triggered, cb);
#endif

    QMainWindow *parent = (QMainWindow*)obs_frontend_get_main_window();
    // action helper, some action must in main thread.
    actionHelpPtr = new ActionHelp(parent);

	tcpServer = new QTcpServer(parent);

	QObject::connect(tcpServer, &QTcpServer::newConnection, actionHelpPtr, &ActionHelp::SDClientConnected);

	if (!tcpServer->listen(QHostAddress::LocalHost, OBS_PORT)) 
	{
		return;
	}

    QLocalSocket *socket = new QLocalSocket();
    if(socket != NULL)
    {
        socket->connectToServer("obsPluginSD");
        socket->deleteLater();
    }

    // setup obs event callback
    obs_frontend_add_save_callback(SaveCallback, nullptr);
    obs_frontend_add_event_callback(OBSEvent, nullptr);
}

// ----------------------------------------------------------------------------
bool obs_module_load(void)
{
    InitStreamDeckPlugin();
    return true;
}

void obs_module_unload(void)
{
    FreeStreamDeckPlugin();
}
