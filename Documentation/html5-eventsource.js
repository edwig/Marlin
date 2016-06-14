/*global net*/

var
    EVENTSOURCE_CONSTANTS_DESCRIPTION,
    EventTarget,
    Event,
    RECONNECTION_TIME, // http://www.w3.org/TR/eventsource/#concept-event-stream-reconnection-time
    DIGITS_REGEX,
    CRLF,
    LF,
    CR,
    connections,
    potentiallyCORSEnabledFetch;



/**
 * @class MessageEvent
 * @extends Event
 * @constructor
 * @param {mixed} data
 * @param {string} origin
 **/
function MessageEvent(data, origin) {
    "use strict";
    Event.call(this, 'message', {
        bubble: false,
        cancellable: false
    });
    this.data = data;
    this.origin = origin;
}
MessageEvent.prototype = Object.create(Event.prototype);



/**
 * @class EventSource
 * @extends EventTarget
 * @constructor
 * @param {string} url
 * @param {EventSourceInit} [eventSourceInitDict]
 **/
function EventSource(url, eventSourceInitDict) {

    "use strict";

    var

        // PROPERTIES
        readyState, // http://www.w3.org/TR/eventsource/#dom-eventsource-readystate
        lastEventId, // http://www.w3.org/TR/eventsource/#concept-event-stream-last-event-id

        // EVENT HANDLERS
        onopen,
        onmessage,
        onerror,

        // BUFFERS
        dataBuffer,
        eventTypeBuffer,
        lastEventIdBuffer,

        // specification defined states
        CORSMode,
        connectionFailed,
        reconnectionTime,
        remoteEventTaskSource, // http://www.w3.org/TR/eventsource/#remote-event-task-source

        // other internal variables
        urlObj,
        nbRetryConnect,
        redirectFlag,
        client;


    /**
     * Queue a task
     *
     * @private
     * @queueATask
     * @param {Function} task
     **/
    function queueATask(task) {
        var
            taskId;

        taskId = setTimeout(function () {
            task();
            remoteEventTaskSource.splice(remoteEventTaskSource.indexOf(taskId), 1);
        }, 0);
        remoteEventTaskSource.push(taskId);
    }


    /**
     * Dispatch the event
     *
     * @private
     * @method dispatchTheEvent
     * @see http://www.w3.org/TR/eventsource/#dispatchMessage
     **/
    function dispatchTheEvent() {
        var
            event,
            ServerEvent;

        // step 1
        lastEventId = lastEventIdBuffer;
        // step 2
        if (dataBuffer === '') {
            // dataBuffer = ''; // specified by the spec but not required as already right
            eventTypeBuffer = '';
            return;
        }
        // step 3
        if (dataBuffer[dataBuffer.length - 1] === LF) {
            dataBuffer.substr(0, dataBuffer.length - 1);
        }
        // step 4 & 5 
        // mixed because step 5 ask to modify the type attribute which is in fact read-only (see DOM Event)
        if (eventTypeBuffer !== '') {
            // create custom ServerEvent
            ServerEvent = function (data, type, origin) {
                Event.call(this, type, {
                    bubble: false,
                    cancellable: false
                });
                this.data = data;
                this.origin = origin;
            };
            ServerEvent.prototype = Object.create(Event.prototype);

            event = new ServerEvent(dataBuffer, eventTypeBuffer, urlObj.absoluteUrl);
        } else {
            // use standard MessageEvent
            event = new MessageEvent(dataBuffer, urlObj.absoluteUrl);
        }
        // step 6
        dataBuffer = '';
        eventTypeBuffer = '';
        // step 7
        queueATask(function dispatchTheEventTask7() {
            if (readyState !== EventSource.CLOSED) {
                this.dispatchEvent(event);
            }
        });
    }


    /**
     * Process the field
     *
     * @private
     * @method processTheField
     * @param {string} field
     * @param {string} value
     * @see http://www.w3.org/TR/eventsource/#processField
     **/
    function processTheField(field, value) {
        switch (field) {

        case 'event':
            eventTypeBuffer = value;
            break;

        case 'data':
            dataBuffer += value + LF;
            break;

        case 'id':
            lastEventIdBuffer = value;
            break;

        case 'retry':
            if (DIGITS_REGEX.test(value)) {
                reconnectionTime = parseInt(value, 10);
            }
            break;

        default:
            // field is ignored
            break;
        }
    }


    /**
     * @private
     * @resolveTheUrl
     * @param {string} url
     * @return Object
     **/
    function resolveTheUrl(url) {
        var
            scheme,
            ssl,
            hostname,
            path,
            host,
            port,
            colonIndex,
            slashIndex;

        url = String(url);
        colonIndex = url.indexOf(':');
        if (colonIndex === -1) {
            throw new SyntaxError('Invalid URL: ' + url);
        }

        scheme = url.substr(0, colonIndex);
        switch (scheme) {
        case 'http':
            ssl = false;
            break;
        case 'https':
            ssl = true;
            break;
        default:
            throw new SyntaxError('invalid sheme: ' + scheme);
        }
        url = url.substr(colonIndex + 3); // pass '://'
        slashIndex = url.indexOf('/');
        if (slashIndex === -1) {
            hostname = url;
            path = '/';
        } else {
            hostname = url.substr(0, slashIndex);
            path = url.substr(slashIndex);
        }
        // raw port detection
        // doesn't support IPV6 addresses nor in URL Basic Auth credentials
        colonIndex = hostname.lastIndexOf(':');
        if (colonIndex === -1) {
            host = hostname;
            port = 80;
        } else {
            host = hostname.substr(0, colonIndex);
            port = Number(hostname.substr(colonIndex));
        }

        return {
            absoluteURL: url,
            scheme: scheme,
            ssl: ssl,
            hostname: hostname,
            path: path,
            host: host,
            port: port
        };
    }


    /**
     * Fail the connection
     *
     * @private
     * @method failTheConnection
     * @param {string} message
     * @see http://www.w3.org/TR/eventsource/#fail-the-connection
     **/
    function failTheConnection(message) {

        function failTheConnectionTask() {
            var
                event;

            if (readyState !== EventSource.CLOSED) {
                readyState = EventSource.CLOSED;
                event = new Event('error');
                event.data = message;
                this.dispatch(event);
                connectionFailed = true;
            }
        }

        queueATask(failTheConnectionTask);
        client.end();
    }


    /**
     * Announce the connection
     *
     * @private
     * @method announceTheConnection
     * @see http://www.w3.org/TR/eventsource/#announce-the-connection
     **/
    function announceTheConnection() {
        var
            openEvent;

        if (readyState !== EventSource.CLOSED) {
            readyState = EventSource.OPEN;
        }

        nbRetryConnect = 0;
        openEvent = new Event('open');

        this.dispatchEvent(openEvent);
    }


    /**
     * @private
     * @method parseTheConnection
     * @param {Array} rows
     * @param {Function} onSuccess
     * @param {Function} onError
     **/
    function parseTheConnection(rows, onSuccess, onError) {
        var
            statusOk,
            acceptOk,
            errorMessage;

        // parse until error found or connection valid and event stream starts
        // (return true to end parseConnection loop)
        function parseConnectionRow(row, index) {
            var
                status,
                redirectURL;

            // STATUS LINE
            if (index === 0) {
                row = row.split(' ');
                if (row[0] !== 'HTTP') {
                    errorMessage = 'Bad response: ' + row;
                    return true;
                }
                status = Number(row[1]);
                switch (status) {
                case 200: // Ok
                    statusOk = true;
                    return false;

                // TODO
                // should be treated transparently as for any other subresource.
                case 305: // Use Proxy
                    errorMessage = 'Needs to use a Proxy (status:' + status + ')';
                    return true;
                case 401: // Unauthorized
                    errorMessage = 'Needs Authentication (status:' + status + ')';
                    errorMessage += LF + rows.filter(function (row) {
                        return (row.substr(0, 12).toUpperCase() === 'AUTHENTICATE');
                    })[0];
                    return true;
                case 407: // Proxy Authentication Required
                    errorMessage = 'Needs Proxy Authentication';
                    return true;

                // handled by the fetching and CORS algorithms. 
                // TODO
                // In the case of 301 redirects, the user agent must also remember the new URL 
                // so that subsequent requests for this resource for this EventSource object 
                // start with the URL given for the last 301 seen for requests for this object.
                case 301: // Moved Permanently
                case 302: // Found
                case 303: // See Other
                case 307: // Temporary Redirect
                    redirectURL = rows.filter(function (row) {
                        return (row.substr(0, 8).toUpperCase() === 'LOCATION');
                    })[0].substr(9).trim();
                    if (!redirectURL) {
                        errorMessage = 'Redirect expected (status: ' + status + ')';
                        errorMessage += ' but no Location found';
                    }
                    urlObj = resolveTheUrl(redirectURL);
                    redirectFlag = true;
                    client.end();
                    potentiallyCORSEnabledFetch(urlObj);
                    rows = [];
                    return true;

                // TODO
                // Those responses, and any network error that prevents the 
                // connection from being established in the first place 
                // (e.g. DNS errors), must cause the client to asynchronously 
                // reestablish the connection.
                case 500: // Internal Server Error
                case 502: // Bad Gateway
                case 503: // Service Unavailable
                case 504: // Gateway Timeout
                    reestablishTheConnection('Server Error (status: ' + status + ')');
                    rows = [];
                    return true;

                default:
                    errorMessage = 'Bad or Unsupported HTTP Response Status: ' + status;
                    return true;
                }
            }

            // CONTENT-TYPE HEADER
            if (row.substr(0, 12).toUpperCase() === 'CONTENT-TYPE') {
                row = row.split(':')[1].trim();
                row = row.substr(0, 17);
                if (row !== 'text/event-stream') {
                    errorMessage = 'Bad MIME Type: ' + row + ' (expected "text/event-stream")';
                    return true;
                }
                acceptOk = true;
                return false;
            }

            // END OF HEADERS
            if (row === '') {
                if (statusOk && acceptOk) {
                    announceTheConnection();
                    rows = rows.splice(0, index);
                    return true;
                }
                errorMessage = 'Bad Response: ' + rows.join(LF);
                return true;
            }

            // IGNORED HEADER
            return false;
        }
        rows.some(parseConnectionRow);

        if (errorMessage) {
            onError(errorMessage);
        } else {
            onSuccess(rows);
        }
    }


    /**
     * Parse the event stream
     *
     * @private
     * @method parseTheEventStream
     * @param {Array} rows
     * @see http://www.w3.org/TR/eventsource/#event-stream-interpretation
     **/
    function parseTheEventStream(rows) {
        rows.forEach(function parseEventStreamRow(row) {
            var
                colonIndex,
                field,
                value;

            // empty line
            if (row === '') {
                dispatchTheEvent();
                return;
            }
            // comment
            if (row[0] === ':') {
                return;
            }
            // field
            colonIndex = row.indexOf(':');
            if (colonIndex !== -1) {
                // with value
                field = row.substr(0, colonIndex);
                value = row.substr(colonIndex);
                if (value[0] === ' ') {
                    value = value.substr(1);
                }
                processTheField(field, value);
            } else {
                // without value
                processTheField(row, '');
            }
        });
    }


    /**
     * Establish the connection
     *
     * @private
     * @method establishTheConnection
     * @see http://www.w3.org/TR/eventsource/#processing-model
     **/
    function establishTheConnection() {

        client = net.connect(urlObj.port, urlObj.host, function onConnectionOpen() {
            var
                request;

            readyState = EventSource.OPEN;

            request = [
                'GET ' + urlObj.path + ' HTTP/1.1',
                'Host: ' + urlObj.host
            ];
            request.push('Accept: text/event-stream');
            request.push('Cache-Control: no-cache');
            if (lastEventId) {
                request.push('Last-Event-ID: ' + lastEventId);
            }
            request.push(''); // end of header

            client.write(request.join(LF));
        });

        // HANDLE SERVER RESPONSE

        client.on('data', function onServerDataReceived(data) {
            var
                eol,
                rows;

            // MANAGE END OF LINE FORMAT (CRLF, CR, or LF)

            if (data.indexOf(CRLF) === -1) {
                if (data.indexOf(LF) === -1) {
                    if (data.indexOf(CR) === -1) {
                        eol = undefined;
                    } else {
                        eol = CR;
                    }
                } else {
                    eol = LF;
                }
            } else {
                eol = CRLF;
            }

            rows = eol ? data.split(eol) : data;

            if (readyState !== EventSource.OPEN) {
                parseTheConnection(
                    rows, // data
                    parseTheEventStream, // onSuccess
                    failTheConnection // onError
                );
            } else {
                parseTheEventStream(rows);
            }

        });


        // HANDLE SERVER CONNECTION CLOSE

        client.on('end', function onConnectionClose() {
            if (!redirectFlag) {
                readyState = EventSource.CLOSED;
            }
        });
    }


    /**
     * Do a potential CORS Enabled Fetch
     *
     * @private
     * @todo Make a real CORS prefetch request
     * @method potentiallyCORSEnabledFetch
     **/
    function potentiallyCORSEnabledFetch(urlObj) {
        // TO BE IMPLEMENTED
        establishTheConnection();
    }


    /**
     * Reestablish the connection
     *
     * @private
     * @method reestablishTheConnection
     * @param {string} [message]
     * @see http://www.w3.org/TR/eventsource/#reestablish-the-connection
     **/
    function reestablishTheConnection(message) {
        var
            task1HasRun;

        function taskStep1() {
            var
                event;

            // step 1.1
            if (readyState === EventSource.CLOSED) {
                return;
            }
            // step 1.2
            readyState = EventSource.CONNECTING;
            // step 1.3
            event = new Event('error');
            event.data = 'reconnecting'; // for a little more detailed error
            if (message) {
                event.data += ' ' + message;
            }
            this.dispatch(event);
            // flag task as done for step 4
            task1HasRun = true;
        }

        function waitTask1HasRun(onTaskOneOver) {

            if (!task1HasRun) {
                setTimeout(waitTask1HasRun, 0);
            }
            onTaskOneOver();
        }

        function taskStep5() {
            // step 5.1
            if (readyState !== EventSource.CONNECTING) {
                return;
            }
            // step 5.2
            potentiallyCORSEnabledFetch(urlObj);
            establishTheConnection();
        }

        function doStep5() {
            queueATask(taskStep5);
        }

        function onceDelayOver() {
            // step 4 & 5
            waitTask1HasRun(doStep5);
        }

        nbRetryConnect += 1;

        // step 1
        queueATask(taskStep1);

        // step 2 & 3
        setTimeout(onceDelayOver, reconnectionTime * nbRetryConnect);

    }

    // EVENT SOURCE OBJECT CONSTRUCTION

    reconnectionTime = RECONNECTION_TIME;

    // http://www.w3.org/TR/eventsource/#dom-eventsource

    // step 1 & 2
    urlObj = resolveTheUrl(url);

    // step 3
    if (!this || !(this instanceof EventSource)) {
        return new EventSource(url, eventSourceInitDict);
    }

    EventTarget.call(this); // EventTarget inheritance

    Object.defineProperties(this, {

        /**
         * The url attribute returns the absolute URL that resulted from resolving the value that was passed 
         * to the constructor.
         *
         * @property url
         * @type string
         * @readonly
         **/
        url: {
            value: urlObj.absoluteURL,
            writable: false,
            enumerable: true
        },

        /**
         * The withCredentials attribute return the value to which it was last initialized.
         * When the object is created, it is initialized to false.
         *
         * @property withCredentials
         * @type boolean
         * @default false
         * @readonly
         **/
        withCredentials: {
            value: eventSourceInitDict && Boolean(eventSourceInitDict.withCredentials),
            writable: false,
            enumerable: true
        },

        /**
         * The readyState attribute represents the state of the connection.
         *
         * It can have the following values:
         *   CONNECTING (numeric value 0)
         *     The connection has not yet been established, or it was closed and the user agent is reconnecting.
         *   OPEN (numeric value 1)
         *     The EventSource has an open connection and is dispatching events as it receives them.
         *   CLOSED (numeric value 2)
         *     The connection is not open, and the user agent is not trying to reconnect. 
         *     Either there was a fatal error or the close() method was invoked.
         *
         * @property readyState
         * @type number
         * @default 0
         * @readonly
         **/
        readyState: {
            get: function getReadyState() { return readyState; },
            set: function setReadyState() { /* readyState is readOnly */ },
            enumerable: true
        },

        /**
         * The connection has been established and received events are dispatched as they are received
         *
         * @event open
         * @property onopen
         * @type Function|null
         * @default null 
         **/
        onopen: {
            get: function getOnOpen() { return onopen || null; },
            set: function setOnOpen(eventHandler) {
                if (typeof onopen === 'function') {
                    this.removeEventListener('open', onopen);
                }
                if (typeof eventHandler === 'function') {
                    onopen = eventHandler;
                    this.addEventListener('open', onopen);
                } else {
                    onopen = null;
                }
            }
        },

        /**
         * A Server MessageEvent has been received
         *
         * @event message
         * @property onmessage
         * @type Function|null
         * @default null 
         **/
        onmessage: {
            get: function getOnMessage() { return onmessage || null; },
            set: function setOnMessage(eventHandler) {
                if (typeof onmessage === 'function') {
                    this.removeEventListener('message', onmessage);
                }
                if (typeof eventHandler === 'function') {
                    onmessage = eventHandler;
                    this.addEventListener('message', onmessage);
                } else {
                    onmessage = null;
                }
            },
            enumerable: true
        },

        /**
         * A error happened (disconnection, bad server response...)
         *
         * @event 
         * @property onerror
         * @type Function|null
         * @default null 
         **/
        onerror: {
            get: function getOnError() { return onerror || null; },
            set: function setOnError(eventHandler) {
                if (typeof onerror === 'function') {
                    this.removeEventListener('error', onerror);
                }
                if (typeof eventHandler === 'function') {
                    onerror = eventHandler;
                    this.addEventListener('error', onerror);
                } else {
                    onerror = null;
                }
            },
            enumerable: true
        }
    });

    /**
     * The close() method abort any instances of the fetch algorithm started for this EventSource object,
     * and set the readyState attribute to CLOSED.
     *
     * @method close
     **/
    this.close = function close() {
        client.end(); // set the readyState attribute to CLOSED
    };

    // step 4
    CORSMode = 'ANONYMOUS';


    // step 5
    if (this.withCredentials) {
        CORSMode = 'USE_CREDENTIALS';
    }


    // CONNECT TO THE SERVER

    readyState = EventSource.CONNECTING;
    nbRetryConnect = 0;
    establishTheConnection();

}


EVENTSOURCE_CONSTANTS_DESCRIPTION = {

    /**
     * The connection has not yet been established, or it was closed and the user agent is reconnecting.
     *
     * @property CONNECTING
     * @type number
     * @default 0
     * @readonly
     **/
    CONNECTING: {
        value: 0,
        writable: false,
        enumerable: true
    },

    /**
     * The EventSource has an open connection and is dispatching events as it receives them.
     *
     * @property OPEN
     * @type number
     * @default 1
     * @readonly
     **/
    OPEN: {
        value: 1,
        writable: false,
        enumerable: true
    },

    /**
     * The connection is not open, and the user agent is not trying to reconnect.
     * Either there was a fatal error or the close() method was invoked.
     *
     * @property CLOSED
     * @type number
     * @default 2
     * @readonly
     **/
    CLOSED: {
        value: 2,
        writable: false,
        enumerable: true
    }
};

Object.defineProperties(EventSource.prototype, EVENTSOURCE_CONSTANTS_DESCRIPTION);
Object.defineProperties(EventSource, EVENTSOURCE_CONSTANTS_DESCRIPTION);

RECONNECTION_TIME = 10000; // 10 seconds

CR = '\r';
LF = '\n';
CRLF = CR + LF;
DIGITS_REGEX = /^[0-9]*$/g;

EventTarget = EventTarget || require('w3c-domevents').EventTarget;
Event = Event || require('w3c-domevents').Event;

connections = [];
remoteEventTaskSource = [];

exports.EventSource = EventSource;
