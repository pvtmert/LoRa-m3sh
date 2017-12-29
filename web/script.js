
'use strict';

let stor = window.localStorage || window.sessionStorage || {
	clear: function() {
		console.log(Object.keys(this))
		return
		for(let k in this) {
			if(typeof this[k] !== "function") {
				this[k] = null
			}
			continue
		}
		return
	},
	set: function(key, value) {
		return (this[key] = value)
	},
	get: function(key, def) {
		return (this[key] || def)
	},
	del: function(key) {
		let val = this[key]
		this[key] = null
		return val
	},
}

Storage.prototype.set = function(key, value) {
	return this.setItem(key, JSON.stringify(value, null, 4))
}
Storage.prototype.get = function(key, def) {
	return (JSON.parse(this.getItem(key) || null) || def)
}
Storage.prototype.del = function(key) {
	let val = this.getItem(key)
	this.removeItem(key)
	return JSON.parse(key || null)
};

let route = {
	exec: function(path, data) {
		for(let r in this) {
			if(path.match(new RegExp(r))) {
				return (this[r])(data)
			}
			continue
		}
		return
	},
}

route["^[/]{0,1}$"] = function(x) {
	hide()
	document.querySelector("#device").classList.remove("d-hide")
	return true
}

let bt = {
	connections: [],
	scan:function(filter, callback) {
		return navigator.bluetooth.requestDevice({
			acceptAllDevices: true,
			optionalServices: filter,
		}).then(callback).catch(console.log)
	},
	connect: function(dev, callback) {
		return dev.gatt.connect().then(callback)
	},
	getServices: function(dev, callback) {
		dev.gatt.getPrimaryServices().then(callback)
		return
	},
	getCharacteristics: function(service, callback) {
		return service.getCharacteristics().then(callback)
	},
	read: function(char, callback) {
		return char.readValue().then(callback)
	},
	write: function(char, value, callback) {
		return char.writeValue(value).then(callback)
	},
	isWritable: function(char) {
		return (char.properties.write || char.properties.writeWithoutResponse)
	},
	isReadable: function(char) {
		return (char.properties.read)
	},
}

window.onhashchange = function(e) {
	let hash = window.location.hash
	if(hash.length < 2 || !hash.startsWith("#!")) {
		if(hash === "#chname") document.querySelector("#edit").value = stor.get(window.chars.addr.value.getUint32().toString(16)).name
		return
	}
	hash = hash.substring(2)
	return route.exec(hash, e)
}

window.onload = function(e) {
	marked.setOptions({
		renderer: new marked.Renderer(),
		gfm: true,
		tables: true,
		breaks: true,
		pedantic: false,
		sanitize: false,
		smartLists: true,
		smartypants: true
	})
	route.exec(window.location.hash.substring(2), e)
	return
}

function hide() {
	let elem = document.querySelector(".off-canvas-content")
	for(let i=0; i<elem.children.length; i++) {
		let self = elem.children[i]
		if(!self.classList.contains("d-hide")) {
			self.classList.add("d-hide")
		}
		continue
	}
	let btn = document.querySelector("#edit")
	if(!btn.classList.contains("d-hide")) {
		btn.classList.add("d-hide")
	}
	return
}

function connect(e) {
	return bt.scan(Object.keys(table), function(dev) {
		console.log("scan:", dev)
		dev.addEventListener("gattserverdisconnected", function(e) {
			hide()
			document.querySelector("#device").classList.remove("d-hide")
			return
		})
		return bt.connect(dev, function(e) {
			console.log("connect:", e)
			return bt.getServices(dev, function(services) {
				console.log("getserv:", services)
				services.forEach(function(serv) {
					console.log("forserv:", serv)
					return bt.getCharacteristics(serv, function(chars) {
						console.log("getchars:", chars)
						chars.forEach(function(char) {
							console.log("forchar:", char)
							let id = {
								s: serv.uuid,
								c: char.uuid,
							}
							if(id.s in table && id.c in table[id.s]) {
								return table[id.s][id.c](char)
							}
							return
						})
						return
					})
				})
				return
			})
		})
	})
	return
}

window.chars = {}
let table = {
	"00001234-0000-1000-8000-00805f9b34fb": {
		"00000001-0000-1000-8000-00805f9b34fb": function(char) {
			char.writeValue(new Uint8Array( [ 0 ] )).then(function(e) {
				char.readValue().then(function(data) {
					if(data.byteLength < 4) {
						return
					}
					data = data.getUint32().toString(16)
					stor.set("self", data)
					document.querySelector("#my").innerHTML = (`
						<kbd>${data}</kbd>
					`)
					hide()
					document.querySelector("#message").classList.remove("d-hide")
					return
				})
				return
			})
			char.startNotifications().then(function(e) {
				char.addEventListener("characteristicvaluechanged", function(event) {
					if(this.value.byteLength < 4) {
						return
					}
					let addr = this.value.getUint32().toString(16)
					if(!stor.get(addr)) {
						stor.set(addr, {
							name: null,
							mesg: {},
						})
					}
					//document.querySelector("#peer").value = addr
					updatePeers()
					return
				})
				return
			})
			window.chars.addr = char
			return
		},
		"00000002-0000-1000-8000-00805f9b34fb": function(char) {
			char.startNotifications().then(function(e) {
				char.addEventListener("characteristicvaluechanged", function(event) {
					let addr = window.chars.addr.value.getUint32().toString(16)
					let hist = stor.get(addr)
					let data = "" //new TextDecoder().decode(this.value)
					let self = this;
					function read(val) {
						if(val.byteLength <= 22) {
							hist.mesg[new Date().getTime()] = data
							stor.set(addr, hist)
							return showMessages(hist.mesg, hist.name || addr)
						} else {
							data += new TextDecoder().decode(val)
							return self.readValue().then(read)
						}
					}
					return read(self.value)
				})
				return
			})
			window.chars.mesg = char
			return
		},
		"00000003-0000-1000-8000-00805f9b34fb": function(char) {
			char.startNotifications().then(function(e) {
				char.addEventListener("characteristicvaluechanged", function(event) {
					let data = new TextDecoder().decode(this.value)
					let addr = "ffffffff"
					let hist = stor.get(addr)
					hist.mesg[new Date().getTime()] = data
					stor.set(addr, hist)
					return
				})
				stor.set("ffffffff", {
					name: "broadcast",
					mesg: {},
				})
				updatePeers()
				return
			})
			return
		},
	},
}

/*
let ex = {
	addr: {
		name: null,
		mesg: {
			date: "content",
		},
	},
}
*/

function showMessages(messages, l, r = "me") {
	let elem = document.querySelector("main.panel .panel-body")
	let scroll = (elem.scrollHeight - elem.offsetHeight == elem.scrollTop)
	elem.innerHTML = ""
	function icon(i) {
		i = i.split(" ")
		i = (i.length > 1 && i.length < 3) ? i[0][0] + i[1][0] : i[0].slice(0,2)
		return (`
			<div class="tile-icon">
				<figure class="avatar avatar-lg" data-initial="${i || "XY"}" >
					<!--i class="avatar-presence online"></i-->
				</figure>
			</div>
		`)
	}
	for(let id in messages) {
		elem.innerHTML += (`
			<div class="tile tile-centered">
				${id > 0 ? icon(l) : ""}
				<div class="tile-content ${id > 0 ? "self-left" : "self-right"}" >
					<div class="tile-title">${id > 0 ? l : r}</div>
					<div class="tile-subtitle text-gray">${new Date(Math.abs(parseInt(id))).toDateString()}</div>
					<div class="tile-content" >${marked(messages[id] || "*no message*")}</div>
				</div>
				${id < 0 ? icon(r) : ""}
				<!-- div class="tile-action">
					<button class="btn btn-link">
						<i class="icon icon-more-vert"></i>
					</button>
				</div -->
			</div>
		`)
		continue
	}
	if(scroll) {
		elem.scrollTop = elem.scrollHeight
	}
	return
}

function updatePeers() {
	let elem = document.querySelector(".nav")
	elem.innerHTML = ""
	for(let peer in stor) {
		if(!peer.match("[0-9a-f]{8}")) {
			continue
		}
		let self = stor.get(peer)
		elem.innerHTML += (`
			<li class="nav-item c-hand" onclick="start('${peer}', this)" >${self.name || peer}</li>
		`)
		continue
	}
}

function start(self, elem) {
	if(elem) {
		document.querySelectorAll(".nav-item").forEach(function(elem) {
			return elem.classList.remove("active")
		})
		elem.classList.add("active")
	}
	if(typeof self !== "string") {
		self = document.querySelector("#peer").value
	}
	stor.set("peer", self || stor.get("peer"))
	hide()
	let addr = stor.get("peer")
	document.querySelector("#edit").classList.remove("d-hide")
	document.querySelector("main.panel").classList.remove("d-hide")
	document.querySelector("main.panel .panel-body").innerHTML = (`<div class="loading loading-lg centered"></div>`)
	window.location.hash = "#"
	let data = stor.get(addr)
	if(!data) {
		stor.set(addr, {
			name: null,
			mesg: {},
		})
		data = stor.get(addr)
		updatePeers()
	}
	let x = parseInt(addr, 16)
	x = ( 0
		| ((((x) >> 24) & 0xff) <<  0)
		| ((((x) >> 16) & 0xff) <<  8)
		| ((((x) >>  8) & 0xff) << 16)
		| ((((x) >>  0) & 0xff) << 24)
	)
	document.querySelector("#msg").focus()
	return window.chars.addr.writeValue(new Uint32Array([ x ])).then(function() {
		return showMessages(data.mesg, data.name || addr)
	})
}

function sendMesg(elem) {
	let data = document.querySelector("#msg").value
	window.chars.mesg.writeValue(new TextEncoder().encode(data)).then(function() {
		let addr = window.chars.addr.value.getUint32().toString(16)
		let hist = stor.get(addr)
		hist.mesg[-(new Date().getTime())] = data
		stor.set(addr, hist)
		showMessages(hist.mesg, hist.name || addr)
		document.querySelector("#msg").value = ""
		return
	})
	return false
}

function chname() {
	let data = document.querySelector("#name").value
	let addr = window.chars.addr.value.getUint32().toString(16)
	let hist = stor.get(addr)
	hist.name = data
	stor.set(addr, hist)
	window.location.hash = '#'
	updatePeers()
	showMessages(hist.mesg, hist.name || addr)
	return
}

function sendLocation(e) {
	navigator.geolocation.getCurrentPosition(function(data) {
		if(!data) {
			return
		}
		data = `[Location ${data.coords.accuracy}](https://www.google.com/maps/place/${data.coords.latitude},${data.coords.longitude})`
		window.chars.mesg.writeValue(new TextEncoder().encode(data)).then(function() {
			let addr = window.chars.addr.value.getUint32().toString(16)
			let hist = stor.get(addr)
			hist.mesg[-(new Date().getTime())] = data
			stor.set(addr, hist)
			showMessages(hist.mesg, hist.name || addr)
			return
		})
		console.log(data)
		return
	})
	e.preventDefault()
	return true
}
