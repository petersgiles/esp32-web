document.documentElement.setAttribute("data-theme", "dracula");

const wsStatus = document.getElementById("wsStatus");
const pinGrid = document.getElementById("pinGrid");
const pinFilter = document.getElementById("pinFilter");
const pinSearch = document.getElementById("pinSearch");
let socket = null;

const pins = [
	{ number: 1, label: "GND", category: "power" },
	{ number: 2, label: "3V3", category: "power" },
	{ number: 3, label: "EN", category: "system" },
	{ number: 4, label: "IO0", category: "system", gpio: 0 },
	{ number: 5, label: "IO1", category: "digital", gpio: 1 },
	{ number: 6, label: "IO2", category: "digital", gpio: 2 },
	{ number: 7, label: "IO3", category: "digital", gpio: 3 },
	{ number: 8, label: "IO4", category: "digital", gpio: 4 },
	{ number: 9, label: "IO5", category: "digital", gpio: 5 },
	{ number: 10, label: "IO6", category: "digital", gpio: 6 },
	{ number: 11, label: "IO7", category: "digital", gpio: 7 },
	{ number: 12, label: "IO8", category: "digital", gpio: 8 },
	{ number: 13, label: "IO9", category: "digital", gpio: 9 },
	{ number: 14, label: "IO10", category: "digital", gpio: 10 },
	{ number: 15, label: "IO11", category: "digital", gpio: 11 },
	{ number: 16, label: "IO12", category: "digital", gpio: 12 },
	{ number: 17, label: "IO13", category: "digital", gpio: 13 },
	{ number: 18, label: "IO14", category: "digital", gpio: 14 },
	{ number: 19, label: "IO15", category: "digital", gpio: 15 },
	{ number: 20, label: "IO16", category: "digital", gpio: 16 },
	{ number: 21, label: "IO17", category: "digital", gpio: 17 },
	{ number: 22, label: "IO18", category: "digital", gpio: 18 },
	{ number: 23, label: "IO19", category: "digital", gpio: 19 },
	{ number: 24, label: "IO20", category: "digital", gpio: 20 },
	{ number: 25, label: "IO21", category: "digital", gpio: 21 },
	{ number: 26, label: "IO26", category: "digital", gpio: 26 },
	{ number: 27, label: "IO33", category: "digital", gpio: 33 },
	{ number: 28, label: "IO34", category: "digital", gpio: 34 },
	{ number: 29, label: "IO35", category: "input", gpio: 35 },
	{ number: 30, label: "IO36", category: "digital", gpio: 36 },
	{ number: 31, label: "IO37", category: "input", gpio: 37 },
	{ number: 32, label: "IO38", category: "digital", gpio: 38 },
	{ number: 33, label: "IO39", category: "digital", gpio: 39 },
	{ number: 34, label: "IO40", category: "digital", gpio: 40 },
	{ number: 35, label: "IO41", category: "digital", gpio: 41 },
	{ number: 36, label: "IO42", category: "digital", gpio: 42 },
	{ number: 37, label: "IO43", category: "digital", gpio: 43 },
	{ number: 38, label: "IO44", category: "digital", gpio: 44 },
	{ number: 39, label: "IO45", category: "digital", gpio: 45 },
	{ number: 40, label: "IO46", category: "digital", gpio: 46 },
	{ number: 41, label: "GND", category: "power" },
];

const pinRuntime = new Map();

function isConnected() {
	return socket && socket.readyState === WebSocket.OPEN;
}

function sendWs(payload) {
	if (!isConnected()) {
		return;
	}
	socket.send(JSON.stringify(payload));
}

function getLiveBadge(gpio) {
	if (gpio === undefined) {
		return '<span class="badge badge-outline">N/A</span>';
	}
	const runtime = pinRuntime.get(gpio);
	if (!runtime) {
		return '<span class="badge badge-warning">Unknown</span>';
	}
	if (runtime.value) {
		return '<span class="badge badge-success">High</span>';
	}
	return '<span class="badge badge-error">Low</span>';
}

function getCategoryBadge(category) {
	if (category === "power") return '<span class="badge badge-neutral">Power</span>';
	if (category === "system") return '<span class="badge badge-secondary">System</span>';
	if (category === "input") return '<span class="badge badge-accent">Live Input</span>';
	return '<span class="badge badge-primary">Digital I/O</span>';
}

function modeLabel(mode) {
	if (mode === "output") return "Output";
	if (mode === "input_pullup") return "Input Pullup";
	return "Input";
}

function getActionButton(pin, runtime) {
	if (pin.gpio === undefined) {
		return '<button class="btn btn-sm btn-outline" disabled>N/A</button>';
	}
	if (!isConnected()) {
		return `<button class="btn btn-sm btn-outline" disabled data-gpio="${pin.gpio}">Offline</button>`;
	}
	if (!runtime) {
		return `<button class="btn btn-sm btn-outline" disabled data-gpio="${pin.gpio}">Wait</button>`;
	}
	if (runtime.mode !== "output") {
		return `<button class="btn btn-sm btn-outline" disabled data-gpio="${pin.gpio}">Set Output</button>`;
	}
	const nextValue = runtime.value ? "0" : "1";
	const buttonText = runtime.value ? "Set Low" : "Set High";
	return `<button class="btn btn-sm btn-outline" data-action="toggle" data-gpio="${pin.gpio}" data-next="${nextValue}">${buttonText}</button>`;
}

function renderPins() {
	if (!pinGrid) return;
	const filterValue = pinFilter?.value ?? "all";
	const query = (pinSearch?.value ?? "").trim().toLowerCase();

	const filteredPins = pins.filter((pin) => {
		const categoryMatch = filterValue === "all" || pin.category === filterValue;
		const searchMatch =
			query.length === 0 ||
			pin.label.toLowerCase().includes(query) ||
			String(pin.number).includes(query);
		return categoryMatch && searchMatch;
	});

	if (filteredPins.length === 0) {
		pinGrid.innerHTML = '<div class="alert">No pins match this filter.</div>';
		return;
	}

	pinGrid.innerHTML = filteredPins
		.map((pin) => {
			const runtime = pin.gpio !== undefined ? pinRuntime.get(pin.gpio) : null;
			const gpioText = pin.label.startsWith("IO") ? `GPIO ${pin.label.slice(2)}` : pin.label;
			const selectedMode = runtime?.mode ?? "input";
			const modeDisabled = pin.gpio === undefined || !isConnected();
			return `
				<div class="pin-card">
					<div class="pin-card-head">
						<div class="font-semibold">${pin.number}: ${pin.label}</div>
						${getLiveBadge(pin.gpio)}
					</div>
					<div class="pin-card-meta">
						${getCategoryBadge(pin.category)}
						<span class="badge badge-outline">${gpioText}</span>
						<span class="badge badge-ghost">Mode: ${modeLabel(selectedMode)}</span>
					</div>
					<div class="pin-card-controls">
						<select class="select select-bordered select-sm" data-action="mode" data-gpio="${pin.gpio ?? ""}" aria-label="Mode for ${pin.label}" ${modeDisabled ? "disabled" : ""}>
							<option value="input" ${selectedMode === "input" ? "selected" : ""}>Input</option>
							<option value="input_pullup" ${selectedMode === "input_pullup" ? "selected" : ""}>Input Pullup</option>
							<option value="output" ${selectedMode === "output" ? "selected" : ""}>Output</option>
						</select>
						${getActionButton(pin, runtime)}
					</div>
				</div>`;
		})
		.join("");
}

function setWsState(state) {
	if (!wsStatus) return;
	if (state === "connected") {
		wsStatus.textContent = "Connected";
		wsStatus.className = "badge badge-success";
		return;
	}
	if (state === "disconnected") {
		wsStatus.textContent = "Disconnected";
		wsStatus.className = "badge badge-error";
		return;
	}
	wsStatus.textContent = "Connecting…";
	wsStatus.className = "badge badge-warning";
}

function connectWebSocket() {
	setWsState("connecting");
	const protocol = window.location.protocol === "https:" ? "wss" : "ws";
	socket = new WebSocket(`${protocol}://${window.location.host}/ws`);
	const connectTimeout = window.setTimeout(() => {
		if (socket.readyState === WebSocket.CONNECTING) {
			setWsState("disconnected");
			socket.close();
		}
	}, 3000);

	socket.addEventListener("open", () => {
		window.clearTimeout(connectTimeout);
		setWsState("connected");
		sendWs({ type: "snapshot" });
		renderPins();
	});

	socket.addEventListener("message", (event) => {
		try {
			const data = JSON.parse(event.data);
			if (data.type === "snapshot" && Array.isArray(data.pins)) {
				for (const item of data.pins) {
					if (typeof item.gpio !== "number") continue;
					pinRuntime.set(item.gpio, {
						mode: typeof item.mode === "string" ? item.mode : "input",
						value: Boolean(item.value),
						canControl: item.canControl !== false,
					});
				}
			}
			renderPins();
		} catch {
			setWsState("disconnected");
		}
	});

	socket.addEventListener("close", () => {
		window.clearTimeout(connectTimeout);
		setWsState("disconnected");
		renderPins();
		window.setTimeout(connectWebSocket, 1000);
	});

	socket.addEventListener("error", () => {
		window.clearTimeout(connectTimeout);
		setWsState("disconnected");
		socket.close();
	});
}

pinGrid?.addEventListener("change", (event) => {
	const target = event.target;
	if (!(target instanceof HTMLSelectElement)) {
		return;
	}
	if (target.dataset.action !== "mode") {
		return;
	}
	const gpio = Number(target.dataset.gpio);
	if (!Number.isInteger(gpio)) {
		return;
	}
	sendWs({ type: "set", gpio, mode: target.value });
});

pinGrid?.addEventListener("click", (event) => {
	const target = event.target;
	if (!(target instanceof HTMLButtonElement)) {
		return;
	}
	if (target.dataset.action !== "toggle") {
		return;
	}
	const gpio = Number(target.dataset.gpio);
	const next = target.dataset.next === "1";
	if (!Number.isInteger(gpio)) {
		return;
	}
	sendWs({ type: "write", gpio, value: next });
});

pinFilter?.addEventListener("change", renderPins);
pinSearch?.addEventListener("input", renderPins);
renderPins();
connectWebSocket();
