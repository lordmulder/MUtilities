///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

//MUtils
#include <MUtils/Startup.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Terminal.h>
#include <MUtils/ErrorHandler.h>
#include <MUtils/Registry.h>
#include <MUtils/Exception.h>

//Qt
#include <QApplication>
#include <QMutex>
#include <QStringList>
#include <QLibraryInfo>
#include <QTextCodec>
#include <QImageReader>
#include <QFont>
#include <QMessageBox>
#include <QtPlugin>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QAbstractNativeEventFilter>
#else
#define QAbstractNativeEventFilter QObject
#define Q_DECL_OVERRIDE
#endif

//CRT
#include <string.h>

//MSVC
#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline
#endif

///////////////////////////////////////////////////////////////////////////////
// Qt Static Initialization
///////////////////////////////////////////////////////////////////////////////

#ifdef QT_NODLL

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
Q_IMPORT_PLUGIN(qico)
Q_IMPORT_PLUGIN(qsvg)
#else
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
#endif

static void doInitializeResources(void)
{
	Q_INIT_RESOURCE(MUtilsData);
}

static void doCleanupResources(void)
{
	Q_CLEANUP_RESOURCE(MUtilsData);
}

namespace MUtils
{
	namespace Startup
	{
		namespace Internal
		{
			class ResourceInitializer
			{
			public:
				ResourceInitializer(void)
				{
					doInitializeResources();
				}

				~ResourceInitializer(void)
				{
					doCleanupResources();
				}
			};

			static ResourceInitializer resourceInitializer;
		}
	}
}

#endif //QT_NODLL

///////////////////////////////////////////////////////////////////////////////
// MESSAGE HANDLER
///////////////////////////////////////////////////////////////////////////////

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
static void qt_message_handler(QtMsgType type, const char *const msg)
{
	if (msg && msg[0])
	{
		MUtils::Terminal::write(type, msg);
		if ((type == QtCriticalMsg) || (type == QtFatalMsg))
		{
			MUtils::OS::fatal_exit(MUTILS_WCHR(QString::fromUtf8(msg)));
		}
	}
}
#else
#define qInstallMsgHandler(X) qInstallMessageHandler((X))
static void qt_message_handler(QtMsgType type, const QMessageLogContext&, const QString &msg)
{
	if (!msg.isEmpty())
	{
		MUtils::Terminal::write(type, msg.toUtf8().constData());
		if ((type == QtCriticalMsg) || (type == QtFatalMsg))
		{
			MUtils::OS::fatal_exit(MUTILS_WCHR(msg));
		}
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
// EVENT FILTER
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Startup
	{
		namespace Internal
		{
			class NativeEventFilter : public QAbstractNativeEventFilter
			{
			public:
				bool nativeEventFilter(const QByteArray&, void *message, long *result) Q_DECL_OVERRIDE
				{
					return filterEvent(message, result);
				};

				static FORCE_INLINE bool filterEvent(void *message, long *result)
				{
					return MUtils::OS::handle_os_message(message, result);
				}

				static NativeEventFilter *instance(void)
				{
					while (m_instance.isNull())
					{
						m_instance.reset(new NativeEventFilter());
					}
					return m_instance.data();
				}

			private:
				NativeEventFilter(void) {}
				static QScopedPointer<MUtils::Startup::Internal::NativeEventFilter> m_instance;
			};
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// STARTUP FUNCTION
///////////////////////////////////////////////////////////////////////////////

static FORCE_INLINE int startup_main(int &argc, char **argv, MUtils::Startup::main_function_t *const entry_point, const char* const appName, const bool &debugConsole)
{
	qInstallMsgHandler(qt_message_handler);
	MUtils::Terminal::setup(argc, argv, appName, MUTILS_DEBUG || debugConsole);
	return entry_point(argc, argv);
}

static FORCE_INLINE int startup_helper(int &argc, char **argv, MUtils::Startup::main_function_t *const entry_point, const char* const appName, const bool &debugConsole)
{
	int iResult = -1;
	try
	{
		iResult = startup_main(argc, argv, entry_point, appName, debugConsole);
	}
	catch(const std::exception &error)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nException error:\n%s\n", error.what());
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}
	catch(...)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nUnknown exception error!\n");
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}
	return iResult;
}

int MUtils::Startup::startup(int &argc, char **argv, main_function_t *const entry_point, const char* const appName, const bool &debugConsole)
{
	int iResult = -1;
#if (MUTILS_DEBUG)
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF || _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif //_MSCVER
	iResult = startup_main(argc, argv, entry_point, appName, debugConsole);
#else //MUTILS_DEBUG
#ifdef _MSC_VER
	__try
	{
		MUtils::ErrorHandler::initialize();
		MUtils::OS::check_debugger();
		iResult = startup_helper(argc, argv, entry_point, appName, debugConsole);
	}
	__except(1)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nUnhandeled structured exception error!\n");
		MUtils::OS::fatal_exit(L"Unhandeled structured exception error, application will exit!");
	}
#else //_MSCVER
	MUtils::ErrorHandler::initialize();
	MUtils::OS::check_debugger();
	iResult = startup_helper(argc, argv, entry_point, appName, debugConsole);
#endif //_MSCVER
#endif //MUTILS_DEBUG
	return iResult;
}

///////////////////////////////////////////////////////////////////////////////
// QT INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

static QMutex g_init_lock;
static const char *const g_imageformats[] = {"bmp", "png", "jpg", "gif", "ico", "xpm", "svg", NULL};

static FORCE_INLINE QString getExecutableName(int &argc, char **argv)
{
	if(argc >= 1)
	{
		const char *argv0 = argv[0];
		for (int i = 0; i < 2; i++)
		{
			static const char SEP[2] = { '/', '\\' };
			if (const char *const ptr = strrchr(argv0, SEP[i]))
			{
				argv0 = ptr + 1;
			}
		}
		if(strlen(argv0) > 1)
		{
			return QString::fromLatin1(argv0);
		}
	}
	return QLatin1String("Program.exe");
}

static FORCE_INLINE void qt_registry_cleanup(void)
{
	static const wchar_t *const QT_JUNK_KEY = L"Software\\Trolltech\\OrganizationDefaults";
	MUtils::Registry::reg_key_delete(MUtils::Registry::root_user, MUTILS_QSTR(QT_JUNK_KEY), true, true);
}

QApplication *MUtils::Startup::create_qt(int &argc, char **argv, const QString &appName, const QString &appAuthor, const QString &appDomain)
{
	QMutexLocker lock(&g_init_lock);
	const OS::ArgumentMap &arguments = MUtils::OS::arguments();

	//Don't initialized again, if done already
	QScopedPointer<QApplication> application(dynamic_cast<QApplication*>(QApplication::instance()));
	if(!application.isNull())
	{
		qWarning("Qt is already initialized!");
		return application.take();
	}

	//Extract executable name from argv[] array
	const QString executableName = getExecutableName(argc, argv);

	//Check Qt version
#ifdef QT_BUILD_KEY
	qDebug("Using Qt v%s [%s], %s, %s", qVersion(), QLibraryInfo::buildDate().toString(Qt::ISODate).toLatin1().constData(), (qSharedBuild() ? "DLL" : "Static"), QLibraryInfo::buildKey().toLatin1().constData());
	qDebug("Compiled with Qt v%s, %s\n", QT_VERSION_STR, QT_BUILD_KEY);
	if(_stricmp(qVersion(), QT_VERSION_STR))
	{
		qFatal("%s", QApplication::tr("Executable '%1' requires Qt v%2, but found Qt v%3.").arg(executableName, QString::fromLatin1(QT_VERSION_STR), QString::fromLatin1(qVersion())).toLatin1().constData());
		return false;
	}
	if(QLibraryInfo::buildKey().compare(QString::fromLatin1(QT_BUILD_KEY), Qt::CaseInsensitive))
	{
		qFatal("%s", QApplication::tr("Executable '%1' was built for Qt '%2', but found Qt '%3'.").arg(executableName, QString::fromLatin1(QT_BUILD_KEY), QLibraryInfo::buildKey()).toLatin1().constData());
		return false;
	}
#else
	qDebug("Using Qt v%s [%s], %s", qVersion(), QLibraryInfo::buildDate().toString(Qt::ISODate).toLatin1().constData(), (qSharedBuild() ? "DLL" : "Static"));
	qDebug("Compiled with Qt v%s\n", QT_VERSION_STR);
#endif

	//Check the Windows version
	const MUtils::OS::Version::os_version_t &osVersion = MUtils::OS::os_version();
	if((osVersion.type != MUtils::OS::Version::OS_WINDOWS) || (osVersion < MUtils::OS::Version::WINDOWS_WINXP))
	{
		qFatal("%s", QApplication::tr("Executable '%1' requires Windows XP or later.").arg(executableName).toLatin1().constData());
	}

	//Check whether we are running on a supported Windows version
	if(const char *const friendlyName = MUtils::OS::os_friendly_name(osVersion))
	{
		qDebug("Running on %s (NT v%u.%u.%u).\n", friendlyName, osVersion.versionMajor, osVersion.versionMinor, osVersion.versionBuild);
	}
	else
	{
		const QString message = QString().sprintf("Running on an unknown WindowsNT-based system (v%u.%u.%u).", osVersion.versionMajor, osVersion.versionMinor, osVersion.versionBuild);
		qWarning("%s\n", MUTILS_UTF8(message));
		MUtils::OS::system_message_wrn(MUTILS_WCHR(executableName), MUTILS_WCHR(message));
	}

	//Check for compat mode
	if(osVersion.overrideFlag && (osVersion <= MUtils::OS::Version::WINDOWS_WN100))
	{
		qWarning("Windows compatibility mode detected!");
		if(!arguments.contains("ignore-compat-mode"))
		{
			qFatal("%s", QApplication::tr("Executable '%1' doesn't support Windows compatibility mode.").arg(executableName).toLatin1().constData());
			return NULL;
		}
	}

	//Check for Wine
	if(MUtils::OS::running_on_wine())
	{
		qWarning("It appears we are running under Wine, unexpected things might happen!\n");
	}

	//Set text Codec for locale
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

	//Create Qt application instance
	application.reset(new QApplication(argc, argv));

	//Register the Qt clean-up function
	atexit(qt_registry_cleanup);

	//Load plugins from application directory
	QCoreApplication::setLibraryPaths(QStringList() << QApplication::applicationDirPath());
	qDebug("Library Path:\n%s\n", MUTILS_UTF8(QApplication::libraryPaths().first()));

	//Set application properties
	application->setApplicationName(appName);
	application->setOrganizationDomain(appDomain);
	application->setOrganizationName(appAuthor);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	application->setEventFilter(&Internal::NativeEventFilter::filterEvent);
#else
	application->installNativeEventFilter(Internal::NativeEventFilter::instance());
#endif

	//Check for supported image formats
	QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
	for(int i = 0; g_imageformats[i]; i++)
	{
		if(!supportedFormats.contains(g_imageformats[i]))
		{
			qFatal("Qt initialization error: QImageIOHandler for '%s' missing!", g_imageformats[i]);
			return NULL;
		}
	}
	
	//Setup console icon
	MUtils::Terminal::set_icon(QIcon(":/mutils/icons/bug.png"));

	//Enable larger/smaller font size
	double fontScaleFactor = 1.0;
	if(arguments.contains("huge-font" )) fontScaleFactor = 1.500;
	if(arguments.contains("big-font"  )) fontScaleFactor = 1.250;
	if(arguments.contains("small-font")) fontScaleFactor = 0.875;
	if(arguments.contains("tiny-font" )) fontScaleFactor = 0.750;
	if(!qFuzzyCompare(fontScaleFactor, 1.0))
	{
		qWarning("Application font scale factor set to: %.3f\n", fontScaleFactor);
		QFont appFont = application->font();
		appFont.setPointSizeF(appFont.pointSizeF() * fontScaleFactor);
		application->setFont(appFont);
	}

	//Check for process elevation
	if(MUtils::OS::is_elevated() && (!MUtils::OS::running_on_wine()))
	{
		QMessageBox messageBox(QMessageBox::Warning, executableName, "<nobr>This program was started with 'elevated' rights, altough it does not need these rights.<br>Running an applications with unnecessary rights is a potential security risk!</nobr>", QMessageBox::NoButton, NULL, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
		messageBox.addButton("Quit Program (Recommended)", QMessageBox::NoRole);
		messageBox.addButton("Ignore", QMessageBox::NoRole);
		if(messageBox.exec() == 0)
		{
			return NULL;
		}
	}

	//QApplication created successfully
	return application.take();
}

///////////////////////////////////////////////////////////////////////////////

