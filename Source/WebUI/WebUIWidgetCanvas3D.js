class WebUIWidgetCanvas3D extends WebUIWidget {
	static template() {
		return [
			{ 'name': "CANVAS 3D", 'control': 'header' },
			{ 'name': 'matrix', 'default': "", 'type': 'source', 'control': 'textedit' },

			{ 'name': "CONTROL", 'control': 'header' },
			{ 'name': 'show_models', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'models', 'default': "", 'type': 'source', 'control': 'textedit' },
			{ 'name': 'show_lines', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'line_color', 'default': "blue", 'type': 'string', 'control': 'textedit' },

			{ 'name': 'show_points', 'default': true, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'point_color', 'default': "black", 'type': 'string', 'control': 'textedit' },
			{ 'name': 'point_size', 'default': "0.15", 'type': 'string', 'control': 'textedit' },

			{ 'name': 'show_axis', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'show_ground_grid', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'show_stats', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'offset_x', 'default': "0", 'type': 'float', 'control': 'textedit' },
			{ 'name': 'offset_y', 'default': "0", 'type': 'float', 'control': 'textedit' },
			{ 'name': 'offset_z', 'default': "0", 'type': 'float', 'control': 'textedit' },

			{ 'name': 'views', 'default': "Home", 'type': 'string', 'control': 'menu', 'options': "Home, Top, Bottom, Front, Back, Left, Right" },
			{ 'name': 'look_at_X', 'default': "0", 'type': 'float', 'control': 'textedit' },
			{ 'name': 'look_at_Y', 'default': "0.8", 'type': 'float', 'control': 'textedit' },
			{ 'name': 'look_at_Z', 'default': "0", 'type': 'float', 'control': 'textedit' },

			{ 'name': "FRAME", 'control': 'header' },
            { 'name': 'show_title', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'show_frame', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'style', 'default': "", 'type': 'string', 'control': 'textedit' },
            { 'name': 'frame-style', 'default': "", 'type': 'string', 'control': 'textedit' },

		]
	};
	static html() {
		return `
		<script type="x-shader/x-vertex" id="vertexshader">
			attribute float size;
			attribute vec3 customColor;
			varying vec3 vColor;
			void main() {
				vColor = customColor;
				vec4 mvPosition = modelViewMatrix * vec4( position, 1.0 );
				gl_PointSize = size * ( 300.0 / -mvPosition.z );
				gl_Position = projectionMatrix * mvPosition;
			}
			</script>
			<script type="x-shader/x-fragment" id="fragmentshader">
			uniform vec3 color;
			uniform sampler2D texture;
			varying vec3 vColor;
			void main() {
				gl_FragColor = vec4( color * vColor, 1.0 );
				gl_FragColor = gl_FragColor * texture2D( texture, gl_PointCoord );
			}
			</script>

		<div class="canvas3d-demo"></div>
		<canvas></canvas>
        `;
	}

	updateAll() 
	{
		this._lastViewName = null;
		super.updateAll();
	}

	init() {
		this.points_loaded = false;
		this.style.position = "relative";
		this.canvasElement = this.querySelector('canvas');
		this.canvasElement.style.display = "block";
		this.canvasElement.style.width = "100%";
		this.canvasElement.style.height = "100%";
		this.canvasElement.style.flexGrow = "1";
		this.canvasElement.style.pointerEvents = "auto";
		this.canvasElement.style.position = "relative";
		this.canvasElement.style.zIndex = "6";
		this.canvas = this.canvasElement.getContext("webgl");
		this.models_loaded = false;
		this._cachedPointColorKey = null;
		this._cachedPointColors = [[0, 0, 0]];
		this._cachedPointSizeKey = null;
		this._cachedPointSizes = [0.15];
		this._cachedLineColorKey = null;
		this._cachedLineColors = [[0, 0, 1]];
		this._cachedLineKey = null;
		this._cachedLineIndices = [];
		this._lastRendererWidth = null;
		this._lastRendererHeight = null;
		this._lastCameraAspect = null;
		this._lastLookAtX = null;
		this._lastLookAtY = null;
		this._lastLookAtZ = null;
		this._lastViewName = null;

		// Scene
		this.scene = new THREE.Scene();
		
		// Light
		this.scene.add( new THREE.AmbientLight( 0xffffff, 0.8 ) );

		const spotLight = new THREE.SpotLight( 0xffffff, 0.5 );
		spotLight.angle = Math.PI / 5;
		//spotLight.penumbra = 0.2;
		spotLight.position.set( 1, 3, 3 );
		spotLight.castShadow = true;
		
		spotLight.shadow.camera.near = 3;
		spotLight.shadow.camera.far = 10;
		spotLight.shadow.mapSize.width = 4096;
		spotLight.shadow.mapSize.height = 4096;
		spotLight.shadowBias = -0.002;
		this.scene.add( spotLight );

		// Camera
		this.camera = new THREE.PerspectiveCamera();
		this.cameraTarget = new THREE.Vector3(this.parameters.look_at_X, this.parameters.look_at_Y, this.parameters.look_at_Z);
		this.camera.aspect = this.parameters.width / this.parameters.height;
		this.camera.position.set(0,0,0);
		
		// Axis
		this.axisHelper = new THREE.AxesHelper(5);
		this.scene.add(this.axisHelper);

		// Ground
		const ground = new THREE.Mesh(
			new THREE.PlaneGeometry( 9, 9, 1, 1 ),
			new THREE.ShadowMaterial( { color: 0x000000, opacity: 0.25, side: THREE.DoubleSide } )
		);
		ground.rotation.x = - Math.PI / 2; // rotates X/Y to X/Z
		ground.position.y = 0;
		ground.receiveShadow = true;
		this.scene.add( ground );

		// Grid
		this.grid = new THREE.GridHelper(200, 200, 0x000000, 0x000000);
		this.grid.material.opacity = 0.2;
		this.grid.material.transparent = true;
		this.scene.add(this.grid);

		// Stats
		this.stats = new Stats();
		this.stats.showPanel(0); // 0: fps, 1: ms, 2: mb, 
		this.s = this.querySelector(".canvas3d-demo");
		if (this.s) {
			this.s.style.position = "absolute";
			this.s.style.top = "0";
			this.s.style.left = "0";
			this.s.style.right = "0";
			this.s.style.bottom = "0";
			this.s.style.pointerEvents = "none";
			this.s.style.flexGrow = "0";
			this.s.style.zIndex = "5";
			this.s.appendChild(this.stats.domElement);
			if (this.s.firstChild) {
				this.s.firstChild.style.position = "absolute";
				this.s.firstChild.style.top = "25px";
				this.s.firstChild.style.left = "5px";
				this.s.firstChild.style.pointerEvents = "none";
			}
		}

		// Renderer
		this.renderer = new THREE.WebGLRenderer({ antialias: true, clearColor: 0x335588, canvas: this.canvas.canvas });
		this.renderer.setClearColor( 0x263238 );
		this.renderer.setPixelRatio(window.devicePixelRatio);
		this.renderer.setSize(this.parameters.width, this.parameters.height);
		this.renderer.shadowMap.enabled = true;
		//this.renderer.shadowMap.cullFace = THREE.CullFaceBack;
		this.renderer.shadowMapType = THREE.PCFSoftShadowMap

		this.controls = new THREE.OrbitControls(this.camera, this.canvasElement);
		this.controls.enablePan = true;
		this.controls.enableRotate = true;
		this.controls.enableZoom = true;
		this.controls.enableDamping = true;
		this.controls.minDistance = 0.1;
		this.controls.maxDistance = 4;
		this.controls.update();

		// Prevent browser/page scrolling while zooming the 3D canvas.
		this.canvasElement.addEventListener("wheel", function (evt) {
			if (evt.cancelable)
				evt.preventDefault();
			evt.stopPropagation();
		}, { passive: false });

		// Safari can occasionally target an overlay sibling instead of the canvas.
		// Forward core mouse/pointer/wheel events to the canvas so OrbitControls always receives input.
		const forwardToCanvas = (evt) => {
			if (!this.canvasElement || evt.target === this.canvasElement || this.canvasElement.contains(evt.target))
				return;
			if (evt.type === "wheel") {
				if (evt.cancelable)
					evt.preventDefault();
				evt.stopPropagation();
				if (evt.stopImmediatePropagation)
					evt.stopImmediatePropagation();
			}
			try {
				let cloned;
				if (typeof PointerEvent !== "undefined" && evt instanceof PointerEvent)
					cloned = new PointerEvent(evt.type, evt);
				else if (typeof WheelEvent !== "undefined" && evt instanceof WheelEvent)
					cloned = new WheelEvent(evt.type, evt);
				else
					cloned = new MouseEvent(evt.type, evt);
				this.canvasElement.dispatchEvent(cloned);
			}
			catch (error) {
			}
		};
		this.addEventListener("pointerdown", forwardToCanvas, true);
		this.addEventListener("pointermove", forwardToCanvas, true);
		this.addEventListener("pointerup", forwardToCanvas, true);
		this.addEventListener("mousedown", forwardToCanvas, true);
		this.addEventListener("mousemove", forwardToCanvas, true);
		this.addEventListener("mouseup", forwardToCanvas, true);
		this.addEventListener("wheel", forwardToCanvas, { capture: true, passive: false });

		// Hard-stop wheel scrolling at widget root as an extra guard.
		this.addEventListener("wheel", function (evt) {
			const targetIsCanvas = evt.target === this.canvasElement || this.canvasElement.contains(evt.target);
			if (evt.cancelable)
				evt.preventDefault();
			// Let wheel continue to OrbitControls when event already targets canvas.
			if (!targetIsCanvas) {
				evt.stopPropagation();
				if (evt.stopImmediatePropagation)
					evt.stopImmediatePropagation();
			}
		}, { capture: true, passive: false });

		function render(o) {
			if (o.controls)
				o.camera.lookAt(o.controls.target);
			else
				o.camera.lookAt(o.cameraTarget);
			o.renderer.render(o.scene, o.camera);
		};

		function animate(o, time) {
			requestAnimationFrame(animate.bind(null, o));
			if (o.controls)
				o.controls.enabled = !main.edit_mode;
			o.controls.update();
			o.stats.update();
			render(o);
		};

		animate(this, 0);
	}

	IkaorsToThreeBase(m) {
		var r1 = new THREE.Matrix4();
		r1.makeRotationY(-Math.PI / 2);
		var r2 = new THREE.Matrix4();
		r2.makeRotationZ(-Math.PI / 2);
		var r = new THREE.Matrix4();
		var ret = new THREE.Matrix4();
		r.multiplyMatrices(r2, r1);
		ret.multiplyMatrices(r, m);
		return ret
	}

	LoadModel(a, name, m) {
		if (!Array.isArray(name) || name.length === 0 || !name[0])
			return;

		//console.log('Loading models')
		var manager = new THREE.LoadingManager();

		manager.onProgress = function (item, loaded, total) {
			console.log(" Progress", item, loaded, total);
		};
		manager.onLoad = function (item, loaded, total) {
			console.log("Everything is loaded");
		};

		manager.onError = function (item, loaded, total) {
			console.log(" Error", item, loaded, total);
		};
		var LoadModels = 0;

		const callback = function (gltf) {
			//console.log("Callback: Loading " + LoadModels + " Name: " + name[LoadModels]);

			a[LoadModels] = gltf.scene
			gltf.scene.traverse(function (child) {
				if (child.isMesh) {
					child.castShadow = true;
					child.receiveShadow = true;
				}
			});

			this.scene.add(gltf.scene);
			LoadModels++;

			if (LoadModels < a.length) // put the next load in the callback
				loader.load('/Models/glb/' + name[LoadModels] + '.glb', callback.bind(this));
		};

		// Instantiate a loader
		var loader = new THREE.GLTFLoader(manager);

		// Load a glTF resource
		loader.load('/Models/glb/' + name[0] + '.glb', callback.bind(this))
		//console.log("Loaded fine")

	}

	updateCameraAndView()
	{
		const parameterWidth = Number(this.parameters.width);
		const parameterHeight = Number(this.parameters.height);
		const clientWidth = this.canvasElement.clientWidth;
		const clientHeight = this.canvasElement.clientHeight;
		const rendererWidth = (Number.isFinite(clientWidth) && clientWidth > 0) ? clientWidth : (Number.isFinite(parameterWidth) ? parameterWidth : this.width);
		const rendererHeight = (Number.isFinite(clientHeight) && clientHeight > 0) ? clientHeight : (Number.isFinite(parameterHeight) ? parameterHeight : this.height);
		const cameraAspect = rendererHeight ? (rendererWidth / rendererHeight) : 1;
		if (this._lastRendererWidth !== rendererWidth || this._lastRendererHeight !== rendererHeight) {
			this.renderer.setSize(rendererWidth, rendererHeight, false);
			this._lastRendererWidth = rendererWidth;
			this._lastRendererHeight = rendererHeight;
		}
		if (this._lastCameraAspect !== cameraAspect) {
			this.camera.aspect = cameraAspect;
			this._lastCameraAspect = cameraAspect;
			this.camera.updateProjectionMatrix();
		}

		const lookAtX = Number(this.parameters.look_at_X);
		const lookAtY = Number(this.parameters.look_at_Y);
		const lookAtZ = Number(this.parameters.look_at_Z);
		const resolvedLookAtX = Number.isFinite(lookAtX) ? lookAtX : 0;
		const resolvedLookAtY = Number.isFinite(lookAtY) ? lookAtY : 0;
		const resolvedLookAtZ = Number.isFinite(lookAtZ) ? lookAtZ : 0;
		if (this._lastLookAtX !== resolvedLookAtX || this._lastLookAtY !== resolvedLookAtY || this._lastLookAtZ !== resolvedLookAtZ) {
			this.cameraTarget.set(resolvedLookAtX, resolvedLookAtY, resolvedLookAtZ);
			if (this.controls)
				this.controls.target.copy(this.cameraTarget);
			this._lastLookAtX = resolvedLookAtX;
			this._lastLookAtY = resolvedLookAtY;
			this._lastLookAtZ = resolvedLookAtZ;
		}

		const viewName = String(this.parameters.views ?? "").trim();
		const shouldApplyPresetView = this._lastViewName !== viewName;
		if (shouldApplyPresetView) {
			switch (viewName) {
				case "Top":
					this.camera.position.set(0, 2, 0);
					break;
				case "Bottom":
					this.camera.position.set(0, -1, 0);
					break;
				case "Front":
					this.camera.position.set(0, 2, 2);
					break;
				case "Back":
					this.camera.position.set(0, 2, -2);
					break;
				case "Left":
					this.camera.position.set(2, 2, 0);
					break;
				case "Right":
					this.camera.position.set(-2, 2, 0);
					break;
				case "Home":
					this.camera.position.set(0.5, 1.5, 1.5);
					break;
				default:
			}
			if (this.controls)
				this.controls.update();
		}
		this._lastViewName = viewName;
		this.FixedView = false;
	}

	update()
	{
		this.updateFrame();
		const incomingData = this.getSource('matrix');
		if (incomingData)
			this.data = incomingData;
		if (!this.data)
		{
			this.updateCameraAndView();
			return;
		}
		if (!Array.isArray(this.data))
		{
			this.updateCameraAndView();
			return;
		}
		if (Array.isArray(this.data) && this.data.length > 0 && !Array.isArray(this.data[0]))
			this.data = [this.data];
		if (!this.data.length)
		{
			this.updateCameraAndView();
			return;
		}

		const parseList = (value) => {
			if (Array.isArray(value)) {
				const flattened = value.flat ? value.flat(Infinity) : value;
				return flattened.map((entry) => String(entry).trim()).filter((entry) => entry !== "");
			}
			const text = String(value ?? "").trim();
			return text === "" ? [] : text.split(",").map((entry) => entry.trim()).filter((entry) => entry !== "");
		};
		const parseColorTriplets = (value, fallback) => {
			const key = String(value ?? "").toLowerCase();
			const list = parseList(key);
			const colors = [];
			for (let i = 0; i < list.length; i++) {
				try {
					const c = new THREE.Color(list[i]);
					colors.push([c.r, c.g, c.b]);
				}
				catch (error) {
				}
			}
			if (colors.length === 0)
				colors.push(fallback);
			return colors;
		};

		const modelsParameter = String(this.parameters.models ?? "");
		if(modelsParameter.includes("@"))	// Minimal fix to load models list from Ikaros
		{
			const modelSource = this.getSource("models");
			if (Array.isArray(modelSource))
				this.parameters.models = (modelSource.flat ? modelSource.flat(Infinity) : modelSource).join(",");
			else
				this.parameters.models = String(modelSource ?? "");
		}

		//console.log("Formating data")
		const offsetX = Number.isFinite(Number(this.parameters.offset_x)) ? Number(this.parameters.offset_x) : 0;
		const offsetY = Number.isFinite(Number(this.parameters.offset_y)) ? Number(this.parameters.offset_y) : 0;
		const offsetZ = Number.isFinite(Number(this.parameters.offset_z)) ? Number(this.parameters.offset_z) : 0;
		this.mat = []
		this.vertices = []
		for (var i = 0; i < this.data.length; i++) {
			if (!Array.isArray(this.data[i]) || this.data[i].length < 16)
				continue;
			this.mat.push(new THREE.Matrix4());
			//this.mat[i].fromArray(this.data[i]) // Not the right order
			this.mat[i].set(this.data[i][0], this.data[i][1], this.data[i][2], this.data[i][3], this.data[i][4], this.data[i][5], this.data[i][6], this.data[i][7], this.data[i][8], this.data[i][9], this.data[i][10], this.data[i][11], this.data[i][12], this.data[i][13], this.data[i][14], this.data[i][15]);
			this.mat[i] = this.IkaorsToThreeBase(this.mat[i]);
			this.mat[i].elements[12] += offsetX;
			this.mat[i].elements[13] += offsetY;
			this.mat[i].elements[14] += offsetZ;
			this.vertices.push(this.mat[i].elements[12], this.mat[i].elements[13], this.mat[i].elements[14])
		}
		this.nrOfModels = this.mat.length
		//console.log("Formating data done")



		// Models
		if (this.toBool(this.parameters.show_models)) {
			//console.log('Models')
			this.modelNames = parseList(this.parameters.models);
			if (this.modelNames.length === 0)
				this.modelNames = ["head"];

			const modelListKey = this.modelNames.join(",");
			if (this.lastmodels != modelListKey) // Remove object if list changed
			{
				if (this.model_objects)
					for (var i = 0; i < this.model_objects.length; i++)
						this.scene.remove(this.model_objects[i]);
				this.models_loaded = false
			}
			this.lastmodels = modelListKey

			// Load models at first update or change of models array
			if (!this.models_loaded) {
				this.widget_loading(true)
				//console.log('Loading models')
				this.model_objects = new Array(this.nrOfModels) 
				for (let i = 0; i < this.nrOfModels; i++)
					this.modelNames[i] = this.modelNames[i % this.modelNames.length];
				this.LoadModel(this.model_objects, this.modelNames, this.data);
				this.models_loaded = true;
			}

			// Update position from an array 16xid
			for (var i = 0; i < this.nrOfModels; i++) {
				if (!this.model_objects[i] || !this.mat[i])
					continue;
				this.model_objects[i].visible = true;
				this.model_objects[i].matrixAutoUpdate = false;
				this.model_objects[i].matrix.copy(this.mat[i])
			}
			if (this.model_objects && this.model_objects.length > this.nrOfModels) {
				for (var i = this.nrOfModels; i < this.model_objects.length; i++) {
					if (this.model_objects[i])
						this.model_objects[i].visible = false;
				}
			}


			//console.log('Updated models')

		}
		else  // Hide 
			if (this.models_loaded)
				if (this.model_objects)
					for (var i = 0; i < this.model_objects.length; i++)
						if (this.model_objects[i])
							this.model_objects[i].visible = false;

		// Remove loading screen
		if (this.models_loaded)
			this.widget_loading(false)

		//console.log("updated")

		// Point
		if (this.toBool(this.parameters.show_points)) {
			//console.log('Points')

			if (!this.points_loaded) {
				this.points_loaded = true;

				var colors = [];
				var color = new THREE.Color();

				sizes = [];

				var geometry = new THREE.BufferGeometry();
				//console.log("Adding points")
				for (var i = 0; i < this.vertices.length; i++) {
					colors.push(0, 0, 0);
					sizes.push(0)
				}
				geometry.setAttribute('position', new THREE.Float32BufferAttribute(this.vertices, 3));
				geometry.setAttribute('customColor', new THREE.Float32BufferAttribute(colors, 3));
				geometry.setAttribute('size', new THREE.Float32BufferAttribute(sizes, 1));

				var material = new THREE.ShaderMaterial({
					uniforms: {
						color: { value: new THREE.Color(0xffffff) },
						texture: { value: new THREE.TextureLoader().load("/Models/Texture/circle.png") }
					},
					vertexShader: document.getElementById('vertexshader').textContent,
					fragmentShader: document.getElementById('fragmentshader').textContent,
					//blending: THREE.AdditiveBlending,
					depthTest: false,
					transparent: true
				});

				this.particles = new THREE.Points(geometry, material);
				this.scene.add(this.particles);

			}

			else {
				//console.log('Updated Points')

				// Calculate point color
				const pointColorKey = String(this.parameters.point_color ?? "").toLowerCase();
				if (this._cachedPointColorKey !== pointColorKey) {
					this._cachedPointColorKey = pointColorKey;
					this._cachedPointColors = parseColorTriplets(this.parameters.point_color, [0, 0, 0]);
				}
				const pointSizeKey = String(this.parameters.point_size ?? "");
				if (this._cachedPointSizeKey !== pointSizeKey) {
					this._cachedPointSizeKey = pointSizeKey;
					this._cachedPointSizes = parseList(this.parameters.point_size).map((entry) => parseFloat(entry)).filter((value) => Number.isFinite(value));
					if (this._cachedPointSizes.length === 0)
						this._cachedPointSizes = [0.15];
				}

				var positions = this.particles.geometry.attributes.position.array;
				var colors = this.particles.geometry.attributes.customColor.array;
				var sizes = this.particles.geometry.attributes.size.array;

				// Update position from an array 16xi
				var cIndex = 0;
				var pIndex = 0
				for (var i = 0; i < this.vertices.length; i++)
					positions[i] = this.vertices[i]

				for (var i = 0; i < this.nrOfModels; i++) {
					// positions[i * 3 + 0] = this.mat[i].elements[12]
					// positions[i * 3 + 1] = this.mat[i].elements[13]
					// positions[i * 3 + 2] = this.mat[i].elements[14]

					if (cIndex >= this._cachedPointColors.length)
						cIndex = 0;
					const color = this._cachedPointColors[cIndex];
					cIndex++;
					colors[i * 3 + 0] = color[0]
					colors[i * 3 + 1] = color[1]
					colors[i * 3 + 2] = color[2]

					if (pIndex >= this._cachedPointSizes.length)
						pIndex = 0;
					sizes[i] = this._cachedPointSizes[pIndex]
					pIndex++;
				}
				this.particles.geometry.attributes.position.needsUpdate = true;
				this.particles.geometry.attributes.customColor.needsUpdate = true;
				this.particles.geometry.attributes.size.needsUpdate = true;
			}
			if (this.particles)
				this.particles.visible = true;
		}
		else  // Hide 
		{
			//console.log("hiding points")
			if (this.points_loaded && this.particles)
				this.particles.visible = false;
		}


		// Line
		if (this.toBool(this.parameters.show_lines)) {
			const lineKey = String(this.parameters.line ?? "");
			if (this._cachedLineKey !== lineKey) {
				this._cachedLineKey = lineKey;
				this._cachedLineIndices = parseList(this.parameters.line).map((entry) => parseInt(entry, 10)).filter((value) => Number.isFinite(value) && value >= 0);
			}
			this.l = this._cachedLineIndices;
			//console.log('Lines')

			if (!this.lines_loaded) {
				this.lines_loaded = true;

				var geometry = new THREE.BufferGeometry();
				var material = new THREE.LineBasicMaterial({ vertexColors: THREE.VertexColors });

				var colors = [];

				for (var i = 0; i < this.vertices.length; i++) {
					colors.push(0, 0, 0);
				}
				geometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
				geometry.setAttribute('position', new THREE.Float32BufferAttribute(this.vertices, 3));
				if (this.l.length > 0)
					geometry.setIndex(new THREE.BufferAttribute(new Uint16Array(this.l), 1));


				this.linesObject = new THREE.LineSegments(geometry, material);
				this.scene.add(this.linesObject);
			}
			else {
				// Calculate point color
				const lineColorKey = String(this.parameters.line_color ?? "").toLowerCase();
				if (this._cachedLineColorKey !== lineColorKey) {
					this._cachedLineColorKey = lineColorKey;
					this._cachedLineColors = parseColorTriplets(this.parameters.line_color, [0, 0, 1]);
				}
				var colors = this.linesObject.geometry.attributes.color.array;

				// Update position from an array 16xi
				var cIndex = 0;

				for (var i = 0; i < this.nrOfModels; i++) {
					if (cIndex >= this._cachedLineColors.length)
						cIndex = 0;
					const color = this._cachedLineColors[cIndex];
					cIndex++;
					colors[i * 3 + 0] = color[0]
					colors[i * 3 + 1] = color[1]
					colors[i * 3 + 2] = color[2]
				}
				this.linesObject.geometry.attributes.color.needsUpdate = true;

				var positions = this.linesObject.geometry.attributes.position.array;

				var geometry = this.linesObject.geometry;
				if (this._lastLineIndexAppliedKey !== this._cachedLineKey) {
					this._lastLineIndexAppliedKey = this._cachedLineKey;
					if (this.l.length > 0)
						geometry.setIndex(new THREE.BufferAttribute(new Uint16Array(this.l), 1));
					else
						geometry.setIndex(null);
				}

				for (var i = 0; i < this.vertices.length; i++)
					positions[i] = this.vertices[i]

				this.linesObject.geometry.attributes.position.needsUpdate = true;
				this.linesObject.visible = true;
				//console.log('Updated Line')

			}
		}
		else  // Hide 
		{
			//console.log("Hiding Line")
			if (this.lines_loaded && this.linesObject)
				this.linesObject.visible = false;
		}

		//console.log("Done lines")

		// Stats
		if (this.toBool(this.parameters.show_stats))
			this.s.firstChild.style.display = "block"
		else
			this.s.firstChild.style.display = "none"

		// Axis
		if (this.toBool(this.parameters.show_axis))
			this.axisHelper.visible = true
		else
			this.axisHelper.visible = false

		// Ground grid
		if (this.toBool(this.parameters.show_ground_grid))
			this.grid.visible = true
		else
			this.grid.visible = false

		this.updateCameraAndView();
		// var t0 = performance.now();
		// var t1 = performance.now();
		// console.log("Update took " + (t1 - t0) + " milliseconds.");
	};
	
};



webui_widgets.add('webui-widget-canvas3d', WebUIWidgetCanvas3D);
