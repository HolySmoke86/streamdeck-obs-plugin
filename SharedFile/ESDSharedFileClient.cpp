#include "ESDSharedFileClient.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QStandardPaths>

// ----------------------------------------------------------------------------
ESDSharedFileClient::ESDSharedFileClient(const QString &key) :
    lockedFlag(false),
    _key(key),
    mapFilePtr(NULL),
    sem(NULL)
{
#if defined(Q_OS_MAC)
    mapfile = QStandardPaths::standardLocations(QStandardPaths::GenericCacheLocation).at(0);
    mapfile.append(QString("/" + _key));

#elif defined(Q_OS_WIN64) || defined(Q_OS_WIN32)
    mapfile = QString(QDir::tempPath() + "/" + _key);
#endif

    mapFileHdl = new QFile(mapfile);

    qDebug() << "mapfile location:" << mapfile;

    open();
}

ESDSharedFileClient::~ESDSharedFileClient()
{
    close();
    delete mapFileHdl;
}

bool ESDSharedFileClient::closeIfNeeded()
{
    QFileInfo f(mapfile);
    if (!f.exists())
    {
        close();
        return true;
    }

    return false;
}

bool ESDSharedFileClient::open()
{
    QString err;

    // create system semaphore
    if (!sem)
    {
        sem = new QSystemSemaphore(_key, 1, QSystemSemaphore::Create);
        if (sem->error() != QSystemSemaphore::NoError)
        {
            err = QString("Error: Unable to create semaphore: %1").arg(sem->errorString());
            qDebug() << err;
            return false;
        }
    }

    // open / create mapfile if not exist.
    QFileInfo f(mapfile);
    if (f.exists())
    {
        if (!mapFileHdl->open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        {
            err = "Error: Could not open SharedFile.";
            qDebug() << err;
            return false;
        }
    }
    else
    {
        err = "Error: SharedFile doesn't exist";
        qDebug() << err;
        return false;
    }

    // increase access count
    if (!attach())
        return false;

    return true;
}

bool ESDSharedFileClient::close()
{
    if (isAttached())
    {
        detach();
    }

    // close map file
    mapFileHdl->close();

    // release system semaphore
    if (sem)
    {
        if (lockedFlag)
        {
            lockedFlag = false;
            sem->release();
        }

        delete sem;
        sem = NULL;
    }

    return true;
}

bool ESDSharedFileClient::attach()
{
    if (isAttached())
    {
        return true;
    }

    if (!mapFileHdl->isOpen())
    {
        qDebug() << "Error: Unable to open or create SharedFile.";
        return false;
    }
	
    mapFilePtr = mapFileHdl->map(0, mapFileHdl->size(), QFileDevice::NoOptions);
    if (!mapFilePtr)
    {
        qDebug() << "Error: Could not map SharedFile.";
        return false;
    }

    return true;
}

void ESDSharedFileClient::detach()
{
    if (!isAttached())
    {
        qDebug() << __FUNCTION__ << "Warning: Asked to detach while not attached.";
        return;
    }

    mapFileHdl->flush();
    mapFileHdl->unmap(mapFilePtr);
    mapFilePtr = NULL;
}

bool ESDSharedFileClient::lock()
{
    if (!isAttached())
    {
        qDebug() << "Warning: Asked to lock while not attached.";
        return false;
    }

    if (!sem->acquire())
    {
        qDebug() << sem->errorString();
        return false;
    }

    lockedFlag = true;
    return true;
}

bool ESDSharedFileClient::unlock()
{
    if (!isAttached())
    {
        qDebug() << "Warning: Asked to unlock while not attached.";
        return false;
    }

    if (!sem->release())
    {
        qDebug() << sem->errorString();
        return false;
    }

    lockedFlag = false;
    return true;
}

bool ESDSharedFileClient::isLocked()
{
    return lockedFlag;
}

bool ESDSharedFileClient::isAttached()
{
    return mapFilePtr;
}

void* ESDSharedFileClient::data()
{
    if (!isAttached())
    {
        qDebug() << "Warning: Asked for data while not attached.";
        return NULL;
    }

    return mapFilePtr;
}

QString ESDSharedFileClient::key()
{
    return _key;
}