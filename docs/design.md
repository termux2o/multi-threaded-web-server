# Design (Short)
- One listener socket on configurable port (default 8080)
- For each accepted connection: spawn a detached std::thread → handle_client()
- handle_client(): read request (4096 bytes), parse GET path (very simple), prevent `..`
- Serve from `www/` (default index.html), return 404 if missing
- MIME type: simple mapping by file extension
- Logging: print path + status to stdout
- Future work: thread pool, non-blocking I/O, better parser, config file
