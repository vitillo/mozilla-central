// Title: utils.js
(function(){

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

// Get this in a way where we can load the page automatically
// where it doesn't need to be focused...
var homeWindow = Cc["@mozilla.org/embedcomp/window-watcher;1"]
    .getService(Ci.nsIWindowWatcher)
    .activeWindow;

var consoleService = Cc["@mozilla.org/consoleservice;1"]
    .getService(Components.interfaces.nsIConsoleService);

var extensionManager = Cc["@mozilla.org/extensions/manager;1"]  
    .getService(Ci.nsIExtensionManager);  

// ##########
// Class: Point
// A simple point. 
//
// Constructor: Point
// If a is a Point, creates a copy of it. Otherwise, expects a to be x, 
// and creates a Point with it along with y. If either a or y are omitted, 
// 0 is used in their place. 
window.Point = function(a, y) {
  if(a && typeof(a.x) != 'undefined' && typeof(a.y) != 'undefined') {
    this.x = a.x;
    this.y = a.y;
  } else {
    this.x = (typeof(a) == 'undefined' ? 0 : a);
    this.y = (typeof(y) == 'undefined' ? 0 : y);
  }
}

window.Point.prototype = { 
  // ---------- 
  distance: function(point) { 
    var ax = Math.abs(this.x - point.x);
    var ay = Math.abs(this.y - point.y);
    return Math.sqrt((ax * ax) + (ay * ay));
  },

  // ---------- 
  plus: function(point) { 
    return new Point(this.x + point.x, this.y + point.y);
  }
};

// ##########  
// Class: Rect
// A simple rectangle. 
//
// Constructor: Rect
// If a is a Rect, creates a copy of it. Otherwise, expects a to be left, 
// and creates a Rect with it along with top, width, and height. 
window.Rect = function(a, top, width, height) {
  // Note: perhaps 'a' should really be called 'rectOrLeft'
  if(typeof(a.left) != 'undefined' && typeof(a.top) != 'undefined'
      && typeof(a.width) != 'undefined' && typeof(a.height) != 'undefined') {
    this.left = a.left;
    this.top = a.top;
    this.width = a.width;
    this.height = a.height;
  } else {
    this.left = a;
    this.top = top;
    this.width = width;
    this.height = height;
  }
}

window.Rect.prototype = {
  // ----------
  get right() {
    return this.left + this.width;
  },
  
  // ----------
  set right(value) {
      this.width = value - this.left;
  },

  // ----------
  get bottom() {
    return this.top + this.height;
  },
  
  // ----------
  set bottom(value) {
      this.height = value - this.top;
  },
  
  // ----------
  intersects: function(rect) {
    return (rect.right > this.left
        && rect.left < this.right
        && rect.bottom > this.top
        && rect.top < this.bottom);      
  },
  
  // ----------
  center: function() {
    return new Point(this.left + (this.width / 2), this.top + (this.height / 2));
  },
  
  // ----------
  inset: function(a, b) {
    if(typeof(a.x) != 'undefined' && typeof(a.y) != 'undefined') {
      b = a.y; 
      a = a.x;
    }
    
    this.left += a;
    this.width -= a * 2;
    this.top += b;
    this.height -= b * 2;
  },
  
  // ----------
  offset: function(a, b) {
    if(typeof(a.x) != 'undefined' && typeof(a.y) != 'undefined') {
      this.left += a.x;
      this.top += a.y;
    } else {
      this.left += a;
      this.top += b;
    }
  },
  
  // ----------
  equals: function(a) {
    return (a.left == this.left
        && a.top == this.top
        && a.right == this.right
        && a.bottom == this.bottom);
  },
  
  union: function(a){
    var newLeft = Math.min(a.left, this.left);
    var newTop = Math.min(a.top, this.top);
    var newWidth = Math.max(a.right, this.right) - newLeft;
    var newHeight = Math.max(a.bottom, this.bottom) - newTop;
    var newRect = new Rect(newLeft, newTop, newWidth, newHeight); 
  
    return newRect;
  },
  
  // ----------
  copy: function(a) {
    this.left = a.left;
    this.top = a.top;
    this.width = a.width;
    this.height = a.height;
  }
};

// ##########
// Class: Subscribable
// A mix-in for allowing objects to collect subscribers for custom events. 
// Currently supports only onClose. 
// TODO generalize for any number of events
window.Subscribable = function() {
  this.onCloseSubscribers = null;
};

window.Subscribable.prototype = {
  // ----------
  // Function: addOnClose
  // The given callback will be called when the Subscribable fires its onClose.
  // The referenceElement is used to facilitate removal if necessary. 
  addOnClose: function(referenceElement, callback) {
    if(!this.onCloseSubscribers)
      this.onCloseSubscribers = [];
      
    var existing = jQuery.grep(this.onCloseSubscribers, function(element) {
      return element.referenceElement == referenceElement;
    });
    
    if(existing.length) {
      Utils.assert('should only ever be one', existing.length == 1);
      existing[0].callback = callback;
    } else {  
      this.onCloseSubscribers.push({
        referenceElement: referenceElement, 
        callback: callback
      });
    }
  },
  
  // ----------
  // Function: removeOnClose
  // Removes the callback associated with referenceElement for onClose notification. 
  removeOnClose: function(referenceElement) {
    if(!this.onCloseSubscribers)
      return;
      
    this.onCloseSubscribers = jQuery.grep(this.onCloseSubscribers, function(element) {
      return element.referenceElement == referenceElement;
    }, true);
  },
  
  // ----------
  // Function: _sendOnClose
  // Internal routine. Used by the Subscribable to fire onClose events.
  _sendOnClose: function() {
    if(!this.onCloseSubscribers)
      return;
      
    jQuery.each(this.onCloseSubscribers, function(index, object) { 
      object.callback(this);
    });
  }
};

// ##########
var Utils = {
  // ___ Windows and Tabs
  get activeWindow(){
    var win = Cc["@mozilla.org/embedcomp/window-watcher;1"]
               .getService(Ci.nsIWindowWatcher)
               .activeWindow;
               
    if( win != null ) return win;  
    else return homeWindow;
  },
  
  get activeTab(){
    var tabBrowser = this.activeWindow.gBrowser;
    return tabBrowser.selectedTab;
  },
  
  get homeTab(){
    for( var i=0; i<Tabs.length; i++){
      if(Tabs[i].contentWindow.location.host == "tabcandy"){
        return Tabs[i];
      }
    }
    
    return null;
  },
    
  // ___ Files
  getInstallDirectory: function(id) { 
    var file = extensionManager.getInstallLocation(id).getItemFile(id, "install.rdf"); 
    return file.parent;  
  }, 
  
  getFiles: function(dir) {
    var files = [];
    if(dir.isReadable() && dir.isDirectory) {
      var entries = dir.directoryEntries;
      while(entries.hasMoreElements()) {
        var entry = entries.getNext();
        entry.QueryInterface(Ci.nsIFile);
        files.push(entry);
      }
    }
    
    return files;
  },

  getVisualizationNames: function() {
    var names = [];
    var dir = this.getInstallDirectory('tabcandy@aza.raskin');  
    dir.append('content');
    dir.append('candies');
    var files = this.getFiles(dir);
    var count = files.length;
    var a;
    for(a = 0; a < count; a++) {
      var file = files[a];
      if(file.isDirectory()) 
        names.push(file.leafName);
    }

    return names;
  },
    
  // ___ Logging
  
  // Interactive logging!
  ilog: function(){ // pass as many arguments as you want, it'll print them all
    // If Firebug lite already exists, print to the console.
    if( window.firebug ){
      window.firebug.d.console.cmd.log.apply(null, arguments);
      return;
    }
    
    // Else, embed it.
    $('<link rel="stylesheet" href="../../js/firebuglite/firebug-lite.css"/>')
      .appendTo("head");
    
    $('<script src="../../js/firebuglite/firebug-lite.js"></script>')
      .appendTo("body");
    
    var args = arguments;
    
    (function(){
      var fb = window.firebug;
      if(fb && fb.version){
        fb.init();
        fb.win.setHeight(100);
        fb.d.console.cmd.log.apply(null, args);
        }
      else{setTimeout(arguments.callee);}
    })();
  },
  
  log: function() { // pass as many arguments as you want, it'll print them all
    var text = this.expandArgumentsForLog(arguments);
    consoleService.logStringMessage(text);
  }, 
  
  error: function() { // pass as many arguments as you want, it'll print them all
    var text = this.expandArgumentsForLog(arguments);
    Cu.reportError('tabcandy error: ' + text);
  }, 
  
  trace: function() { // pass as many arguments as you want, it'll print them all
    var text = this.expandArgumentsForLog(arguments);
    if(typeof(printStackTrace) != 'function')
      this.log(text + ' trace: you need to include stacktrace.js');
    else {
      var calls = printStackTrace();
      calls.splice(0, 3); // Remove this call and the printStackTrace calls
      this.log('trace: ' + text + '\n' + calls.join('\n'));
    }
  }, 
  
  assert: function(label, condition) {
    if(!condition) {
      var text;
      if(typeof(label) == 'undefined')
        text = 'badly formed assert';
      else
        text = 'tabcandy assert: ' + label;        
        
      if(typeof(printStackTrace) == 'function') {
        var calls = printStackTrace();
        text += '\n' + calls[3];
      }
      
      Cu.reportError(text);
    }
  },
  
  expandObject: function(obj) {
      var s = obj + ' = {';
      for(prop in obj) {
        var value;
        try {
          value = obj[prop]; 
        } catch(e) {
          value = '[!!error retrieving property]';
        }
        
        s += prop + ': ';
        if(typeof(value) == 'string')
          s += '\'' + value + '\'';
        else if(typeof(value) == 'function')
          s += 'function';
        else
          s += value;

        s += ", ";
      }
      return s + '}';
    }, 
    
  expandArgumentsForLog: function(args) {
    var s = '';
    var count = args.length;
    var a;
    for(a = 0; a < count; a++) {
      var arg = args[a];
      if(typeof(arg) == 'object')
        arg = this.expandObject(arg);
        
      s += arg;
      if(a < count - 1)
        s += '; ';
    }
    
    return s;
  },
  
  testLogging: function() {
    this.log('beginning logging test'); 
    this.error('this is an error');
    this.trace('this is a trace');
    this.log(1, null, {'foo': 'hello', 'bar': 2}, 'whatever');
    this.log('ending logging test');
  }, 
  
  // ___ Event
  isRightClick: function(event) {
    if(event.which)
      return (event.which == 3);
    else if(event.button) 
      return (event.button == 2);
    
    return false;
  },
  
  // ___ Time
  getMilliseconds: function() {
  	var date = new Date();
  	return date.getTime();
  },
  
  // ___ Geometry
  getBounds: function(el) {
    var $el = $(el);
    return new Rect(
      parseInt($el.css('left')), 
      parseInt($el.css('top')),
      $el.width(),
      $el.height()
    );
  },

  // ___ Misc
  isJQuery: function(object) {
    // TODO: need more robust way 
    return (object && typeof(object.fadeIn) == 'function' ? true : false);
  },   

  isDOMElement: function(object) {
    // TODO: need more robust way 
    return (object && typeof(object.tagName) != 'undefined' ? true : false);
  }   
};

window.Utils = Utils;

window.Math.tanh = function tanh(x){
  var e = Math.exp(x);
  return (e - 1/e) / (e + 1/e); 
}

})();
