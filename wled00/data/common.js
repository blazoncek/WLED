var d=document;
var loc = false, locip, locproto = "http:";

function H(p="")    { window.open("https://kno.wled.ge"+p); }
function GH(p="")   { window.open("https://github.com/wled/WLED"+p); }
function gId(c)     { return d.getElementById(c); } // getElementById
function cE(e)      { return d.createElement(e); } // createElement
function gEBCN(c)   { return d.getElementsByClassName(c); } // getElementsByClassName
function gEBN(s)    { return d.getElementsByName(s); } // getElementsByName
function gN(s)      { return gEBN(s)[0]; } // getElementsByName[0]
function qSA(s)     { return d.querySelectorAll(s); } // querySelectorAll
function isE(o)     { return Object.keys(o).length === 0; } // isEmpty
function isO(i)     { return (i && typeof i === 'object' && !Array.isArray(i)); } // isObject
function isN(n)     { return !isNaN(parseFloat(n)) && isFinite(n); } // isNumber
// https://stackoverflow.com/questions/3885817/how-do-i-check-that-a-number-is-float-or-integer
function isF(n)     { return n === +n && n !== (n|0); } // isFloat
function isI(n)     { return n === +n && n === (n|0); } // isInteger
function chrID(x)   { return String.fromCharCode((x<10?48:55)+x); }
function toggle(el) { gId(el).classList.toggle("hide"); let n = gId('No'+el); if (n) n.classList.toggle("hide"); }
function tooltip(cont=null) {
	qSA((cont?cont+" ":"")+"[title]").forEach((element)=>{
		element.addEventListener("mouseover", ()=>{
			// save title
			element.setAttribute("data-title", element.getAttribute("title"));
			const tooltip = cE("span");
			tooltip.className = "tooltip";
			tooltip.textContent = element.getAttribute("title");

			// prevent default title popup
			element.removeAttribute("title");

			let { top, left, width } = element.getBoundingClientRect();

			d.body.appendChild(tooltip);

			const { offsetHeight, offsetWidth } = tooltip;

			const offset = element.classList.contains("sliderwrap") ? 4 : 10;
			top -= offsetHeight + offset;
			left += (width - offsetWidth) / 2;

			tooltip.style.top = top + "px";
			tooltip.style.left = left + "px";
			tooltip.classList.add("visible");
		});

		element.addEventListener("mouseout", ()=>{
			qSA('.tooltip').forEach((tooltip)=>{
				tooltip.classList.remove("visible");
				d.body.removeChild(tooltip);
			});
			// restore title
			element.setAttribute("title", element.getAttribute("data-title"));
		});
	});
};
// sequential loading of external resources (JS or CSS) with retry, calls init() when done
function loadResources(files, init = undefined) {
	let j = [], c = [];
	files.forEach(e=>{
		let x = e.substring(e.lastIndexOf('.')+1);
		if (x == 'css') c.push(e);
		if (x == 'js') j.push(e);
	});
	loadJS(j, false, ()=>{loadCss(c)}, init);
}
function loadCss(files) {
	var r = 3;
	const next = () => {
		if (files.length) {
			const file = files.shift();
			let ext = file.substring(file.lastIndexOf('.')+1)
			if (ext.substring(0,3) == 'css') {
				let el = cE('link');
				el.rel = 'stylesheet';
				el.href = file.substring(0,4) == 'http' ? file : getURL(file);
				el.onload = () => {
					r = 3;
					setTimeout(next,10); // give ESP some slack
				};
				el.onerror = () => {
					if (r--) {
						files.unshift(file); // retry
						setTimeout(next, 150);
					}
				};
				const st = d.head.querySelector('style');
				if (st) d.head.insertBefore(el, st); // insert before any <style> to allow overrides
				else d.head.appendChild(el);
			}
		}
	};
	next();
}
// https://www.educative.io/edpresso/how-to-dynamically-load-a-js-file-in-javascript
function loadJS(files, async = true, preGetV = undefined, postGetV = undefined) {
	if (!Array.isArray(files)) files = [files]; // compatibility with old API
	var r = 3;
	const next = () => {
		if (files.length) {
			const file = files.shift();
			let ext = file.substring(file.lastIndexOf('.')+1)
			if (ext.substring(0,2) == 'js') {
				let el = cE('script');
				el.async = async;
				el.src = file.substring(0,4) == 'http' ? file : getURL(file);
				el.type = "text/javascript";
				el.onload = () => {
					r = 3; // reset retry counter
					setTimeout(next,10); // give ESP some slack
				};
				el.onerror = (e) => {
					console.log("Error loading JS file", e);
					// retry & reduce retry count
					if (r--) {
						files.unshift(file);
						setTimeout(next, 150);
					} else {
						alert("Loading script failed.\nIncomplete page data!");
					}
				};
				d.body.appendChild(el);
			}
		} else if (!async) {
			if (preGetV) preGetV();
			try { GetV(); } catch (e) {} // GetV() injected by settings script
			if (postGetV) postGetV();
		}
	};
	next();
}
function getLoc() {
	let l = window.location;
	if (l.protocol == "file:") {
		loc = true;
		locip = localStorage.getItem('locIp');
		if (!locip) {
			locip = prompt("File Mode. Please enter WLED IP!");
			localStorage.setItem('locIp', locip);
		}
	} else {
		// detect reverse proxy
		let paths = l.pathname.split("/").slice(1); // first is always empty
		let settingsIndex = paths.lastIndexOf("settings"); // -1 or index of "settings"
		paths = paths.slice(0,settingsIndex); // if we don't have "settings", remove last entry (empty / or file)
		if (paths.length > 1) {
			locproto = l.protocol;
			loc = true;
			locip = l.hostname + (l.port ? ":" + l.port : "") + "/" + paths.join('/');
		}
	}
}
function getURL(path) { return (loc ? locproto + "//" + locip : "") + path; }
function B()          { window.open(getURL("/settings"),"_self"); }
var timeout;
function showToast(text, error = false) {
	var x = gId("toast");
	if (!x) return;
	x.innerHTML = text;
	x.className = error ? "error":"show";
	clearTimeout(timeout);
	x.style.animation = 'none';
	timeout = setTimeout(function(){ x.className = x.className.replace("show", ""); }, 2900);
}
function uploadFile(fileObj, name) {
	var req = new XMLHttpRequest();
	req.onload = ()=>{showToast(this.responseText,this.status >= 400);};
	req.onerror = (e)=>{showToast(e.stack,true);};
	req.open("POST", getURL("/upload"));
	var formData = new FormData();
	formData.append("data", fileObj.files[0], name);
	req.send(formData);
	fileObj.value = '';
	return false;
}
// connect to WebSocket, use parent WS or open new, callback function gets passed the new WS object
function connectWs(onOpenMsg) {
	let ws = top?.window?.ws;
	// reuse if open
	if (ws && ws.readyState === WebSocket.OPEN) {
		if (onOpenMsg) ws.send(onOpenMsg);
	} else {
		// create new ws connection
		getLoc(); // ensure globals are up to date
		let url = loc ? getURL('/ws').replace("http", "ws") : "ws://" + window.location.hostname + "/ws";
		ws = new WebSocket(url);
		ws.binaryType = "arraybuffer";
		if (onOpenMsg) ws.onopen = () => ws.send(onOpenMsg);
	}
	return ws;
}
