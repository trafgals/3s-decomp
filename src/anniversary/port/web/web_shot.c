// Uploading a file of the build to the dev server.
//
// The browser build blocks the main thread while the game is running, so the
// page cannot read the canvas back to take a screenshot. The build takes the
// screenshot itself and sends it to the dev server, which writes it to a
// file.

#include "common.h"
#include "port/web/web_shot.h"

#include <emscripten/emscripten.h>

EM_JS(void, web_upload_send, (const char* path, const char* name), {
    try {
        var data = FS.readFile(UTF8ToString(path));
        var blob = new Blob([data], { type: 'application/octet-stream' });
        var url = '/__shot?name=' + encodeURIComponent(UTF8ToString(name));
        // sendBeacon() refuses the payloads that are bigger than a few
        // kilobytes, and a screenshot is a lot bigger than that.
        fetch(url, { method: 'POST', body: blob }).then(function () {
            console.log('uploaded ' + UTF8ToString(name));
        }, function (err) {
            console.error('upload of ' + UTF8ToString(name) + ' failed: ' + err);
        });
    } catch (err) {
        console.error('web_upload_send failed: ' + err);
    }
});

void web_upload_file(const char* path, const char* name) {
    web_upload_send(path, name);
}
