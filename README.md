# media.exe

This is a simple tool to be able to display the currently playing media from the
commandline. It makes use of the
[`Windows.Media.Control`](https://docs.microsoft.com/en-us/uwp/api/windows.media.control.globalsystemmediatransportcontrolssession)
APIs to be able to query the media that's currently playing. This application
doesn't actually play any media itself, it merely displays the information
provided from another application. So if you're using an app like Spotify or
iTunes that uses the system media transport controls (SMTC) to share information
about the currently playing media, this appllication will be able to display
that information.


