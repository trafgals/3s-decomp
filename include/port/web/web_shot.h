#ifndef WEB_SHOT_H
#define WEB_SHOT_H

/*
 * Uploads a file of the file system of the build to the dev server.
 *
 * The game loop of the browser build blocks the main thread, so a screenshot
 * cannot be taken from the JavaScript side: the page is not able to read the
 * canvas back. The build therefore takes the screenshot itself and sends it
 * to the dev server, which writes it to a file that the verification script
 * can look at.
 */

void web_upload_file(const char* path, const char* name);

#endif
