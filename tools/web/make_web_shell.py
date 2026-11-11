#!/usr/bin/env python3
"""Generate the HTML shell for the web build.

The emscripten module is loaded as a classic script, so the page has to
provide the `Module` object it expects. The shell also owns the canvas that
SDL3 renders into and shows a loading overlay until the runtime is up.

Usage:
    python3 tools/web/make_web_shell.py build/anniversary/web/sf33rd.js \\
        build/anniversary/web/index.html
"""

import sys
from pathlib import Path

TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>Street Fighter III: 3rd Strike{title_suffix}</title>
<style>
  html, body {{
    margin: 0;
    padding: 0;
    height: 100%;
    background: #101014;
    color: #e8e8ea;
    font-family: system-ui, -apple-system, "Segoe UI", sans-serif;
    overflow: hidden;
  }}
  #stage {{
    position: absolute;
    inset: 0;
    display: flex;
    align-items: center;
    justify-content: center;
  }}
  canvas {{
    display: block;
    image-rendering: pixelated;
    outline: none;
    background: #000;
    max-width: 100vw;
    max-height: 100vh;
  }}
  #overlay {{
    position: absolute;
    inset: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 0.75rem;
    background: rgba(8, 8, 12, 0.92);
    text-align: center;
    padding: 2rem;
  }}
  #overlay.hidden {{ display: none; }}
  #status {{ font-size: 1rem; opacity: 0.85; }}
  #error {{
    max-width: 48rem;
    white-space: pre-wrap;
    font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
    font-size: 0.8rem;
    color: #ff8a8a;
    text-align: left;
  }}
  #hint {{ font-size: 0.85rem; opacity: 0.6; }}
</style>
</head>
<body>
<div id="stage"><canvas id="canvas" tabindex="0" width="1280" height="960"></canvas></div>
<div id="overlay">
  <div id="status">Loading Third Strike...</div>
  <div id="error"></div>
  <div id="hint">Click the screen to give it keyboard focus.</div>
</div>
<script>
  // The game reports its own failures on stderr; keep them so the overlay
  // can show the actual reason instead of an empty abort message.
  var errorLog = [];

  var Module = {{
    canvas: document.getElementById('canvas'),
    print: function (text) {{ console.log(text); }},
    printErr: function (text) {{
      console.error(text);
      errorLog.push(String(text));
    }},
    onRuntimeInitialized: function () {{
      var overlay = document.getElementById('overlay');
      overlay.classList.add('hidden');
      var canvas = document.getElementById('canvas');
      canvas.focus();
    }},
    onAbort: function (what) {{
      fail('The game aborted: ' + (what || describeError()));
    }},
  }};

  function describeError() {{
    var all = errorLog.join('\\n');
    var fatal = all.match(/Fatal error:[^\\n]*/);
    if (fatal) {{
      return fatal[0];
    }}
    var lines = all.split('\\n').filter(function (part) {{
      return part.trim().length > 0;
    }});
    return lines.length ? lines[0] : 'no error message';
  }}

  function fail(message) {{
    var overlay = document.getElementById('overlay');
    var status = document.getElementById('status');
    var error = document.getElementById('error');
    overlay.classList.remove('hidden');
    status.textContent = 'Something went wrong';
    error.textContent = message;
  }}

  // The game blocks the main thread of the page, so the dev server is the
  // only place where its output can be read from the outside. Every line the
  // game prints and every error the page runs into is forwarded to it, which
  // is what makes a stall diagnosable.
  //
  // The report goes out with a beacon instead of a synchronous request: the
  // game runs inside a fiber, and a request that waits for its answer would
  // block the page while the game is in the middle of a fiber switch.
  function report(line) {{
    try {{
      navigator.sendBeacon('/__trace?t=' + encodeURIComponent('console: ' + line));
    }} catch (e) {{
      // The dev server is optional, so a failure to report is not fatal.
    }}
  }}

  ['log', 'info', 'warn', 'error'].forEach(function (name) {{
    var original = console[name].bind(console);
    console[name] = function () {{
      report(Array.prototype.join.call(arguments, ' '));
      original.apply(null, arguments);
    }};
  }});

  window.addEventListener('error', function (event) {{
    report('page error: ' + (event.message || describeError()));
    if (event.message) {{ fail(event.message); }}
  }});

  window.addEventListener('unhandledrejection', function (event) {{
    var reason = event.reason;
    var text = 'unhandled rejection: ' + reason;
    if (reason && reason.stack) {{
      text += ' | ' + reason.stack;
    }}
    report(text);
  }});

  document.getElementById('canvas').addEventListener('click', function () {{
    this.focus();
  }});
</script>
<script src="{script_name}"></script>
</body>
</html>
"""


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 1

    script_path = Path(sys.argv[1])
    output_path = Path(sys.argv[2])
    output_path.parent.mkdir(parents=True, exist_ok=True)

    suffix = " - Third Strike Upper (3s')" if "upper" in script_path.stem.lower() else ""
    output_path.write_text(
        TEMPLATE.format(script_name=script_path.name, title_suffix=suffix),
        encoding="utf-8",
    )
    print(f"wrote {output_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
