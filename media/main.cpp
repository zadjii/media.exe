#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace winrt::Windows::Media;

struct Instance
{
    winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession
        session{ nullptr };

	bool HasMediaSession()
	{
		return session != nullptr;
	}

    void TogglePlayPause()
    {
        if (session)
        {
            auto info = session.GetPlaybackInfo();
            auto playbackState = info.PlaybackStatus();

            if (playbackState == Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing)
            {
                session.TryPauseAsync();
            }
            else if (playbackState == Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused)
            {
                session.TryPlayAsync();
            }
        }
    };
    void NextTrack()
    {
        if (session)
        {
            auto info = session.GetPlaybackInfo();
            auto playbackState = info.PlaybackStatus();

            if (playbackState == Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing)
            {
                session.TrySkipNextAsync();
            }
        }
    };
    void PrevTrack()
    {
        if (session)
        {
            auto info = session.GetPlaybackInfo();
            auto playbackState = info.PlaybackStatus();

            if (playbackState == Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing)
            {
                session.TrySkipPreviousAsync();
            }
        }
    };
};

struct RealtimeDisplay
{
    HANDLE hIn = 0;
    HANDLE hOut = 0;
    bool exitRequested = false;
    Instance instance;
    std::wstring originalTitle{};

    void SetupConsole()
    {
        hIn = GetStdHandle(STD_INPUT_HANDLE);
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD outModes = 0;
        GetConsoleMode(hOut, &outModes);
        outModes |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, outModes);

        wchar_t titleBuffer[1024];
        GetConsoleTitle(titleBuffer, 1024);
        originalTitle = titleBuffer;

        printf("\x1b[?1049h");
    }

    void Prompt(const bool clear = true)
    {
        printf("\x1b[7H");
        if (clear)
        {
            printf("\x1b[K");
        }
    };

    void KeyHandler(const KEY_EVENT_RECORD& key)
    {
        if (key.bKeyDown)
        {
            if (key.uChar.UnicodeChar == L'x')
            {
                exitRequested = true;
                Prompt();
                printf("exit");
            }

            if (key.uChar.UnicodeChar == L' ')
            {
                Prompt();
                printf("play/pause");
                instance.TogglePlayPause();
            }

            if (key.uChar.UnicodeChar == L'n')
            {
                Prompt();
                printf("next");
                instance.NextTrack();
            }
            if (key.uChar.UnicodeChar == L'p')
            {
                Prompt();
                printf("prev");
                instance.PrevTrack();
            }
        }
    }

    void DisplayInfo(const Control::GlobalSystemMediaTransportControlsSession& session)
    {
        printf("\x1b[H");
        auto mediaAsync = session.TryGetMediaPropertiesAsync();
        auto media = mediaAsync.get();
        auto artist = media.AlbumArtist();
        auto title = media.Title();
        auto info = session.GetPlaybackInfo();
        auto timeline = session.GetTimelineProperties();
        auto pos = timeline.Position();
        auto end = timeline.EndTime();

        auto status = info.PlaybackStatus();
        printf("\x1b[K%ls\n", title.c_str());
        printf("\x1b]2;%ls\x1b/", title.c_str());
        printf("\x1b[K%ls\n", artist.c_str());
        printf("\x1b[K");
        printf("( ): ");

        if (status == Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing)
        {
            printf("pause");
        }
        else if (status == Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused)
        {
            printf("play");
        }
        printf("\n");

        printf("(p): Previous Track");
        printf("\n");
        printf("(n): Next Track");
        printf("\n");
        printf("(x): exit");

        Prompt();
    };

    void SetUpEvents()
    {
        instance.session.MediaPropertiesChanged([this](const Control::GlobalSystemMediaTransportControlsSession& session,
                                                       const Control::MediaPropertiesChangedEventArgs& args) {
            DisplayInfo(session);
        });
        instance.session.PlaybackInfoChanged([this](const Control::GlobalSystemMediaTransportControlsSession& session,
                                                    const Control::PlaybackInfoChangedEventArgs& args) {
            DisplayInfo(session);
        });
        instance.session.TimelinePropertiesChanged([this](const Control::GlobalSystemMediaTransportControlsSession& session,
                                                    const Control::TimelinePropertiesChangedEventArgs& args) {
            DisplayInfo(session);
        });
    }

    void Start()
    {
        SetupConsole();
        SetUpEvents();
        DisplayInfo(instance.session);
        ProcessInput();
        Exit();
    }

    void ProcessInput()
    {
        while (!exitRequested)
        {
            INPUT_RECORD input[16];
            DWORD numRead = 0;
            ReadConsoleInput(hIn, input, 16, &numRead);
            for (auto i = 0; i < numRead; i++)
            {
                switch (input[i].EventType)
                {
                case KEY_EVENT: // keyboard input
                {
                    KeyHandler(input[i].Event.KeyEvent);
                    break;
                }
                }
            }
        }
    }

    void Exit()
    {
        Prompt(false);
        printf("\n");

        printf("\x1b[31m");
        printf("Exiting...");
        printf("\x1b[m");
        Sleep(1000);
        SetConsoleTitle(originalTitle.c_str());
        printf("\x1b[?1049l");
    }
};

Instance SetUpMediaSession()
{
    auto request = winrt::Windows::Media::Control::
        GlobalSystemMediaTransportControlsSessionManager::RequestAsync();
    auto mgr = request.get();

    Instance current{};
    current.session = mgr.GetCurrentSession();
    return current;
}

void DisplayCurrent(Instance& mediaInstace)
{
    auto mediaAsync = mediaInstace.session.TryGetMediaPropertiesAsync();
    auto media = mediaAsync.get();
    auto artist = media.AlbumArtist();
    auto title = media.Title();

    printf("%ls\n", title.c_str());
    printf("%ls\n", artist.c_str());
}

void DisplayRealtime(Instance& mediaInstace)
{
    RealtimeDisplay program;
    program.instance = mediaInstace;
    program.Start();
}

int main()
{
    init_apartment();

    auto instance = SetUpMediaSession();
	if (!instance.HasMediaSession())
	{
		printf("No media currently playing.\n");
		return 0;
	}
    // DisplayCurrent(instance);
    DisplayRealtime(instance);
}
