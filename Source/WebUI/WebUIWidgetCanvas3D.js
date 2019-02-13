class WebUIWidgetCanvas3D extends WebUIWidget {
	static template() {
		return [
			{ 'name': "CANVAS 3D", 'control': 'header' },
			{ 'name': 'module', 'default': "", 'type': 'source', 'control': 'textedit' },
			{ 'name': 'source', 'default': "", 'type': 'source', 'control': 'textedit' },
			{ 'name': 'title', 'default': "", 'type': 'string', 'control': 'textedit' },
			{ 'name': 'show_title', 'default': true, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'show_frame', 'default': true, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'camera1', 'default':0, 'type':'float', 'control': 'textedit'},
			{ 'name': 'camera2', 'default':2, 'type':'float', 'control': 'textedit'},
			{ 'name': 'camera3', 'default':1, 'type':'float', 'control': 'textedit'}
		]
	};

	static html() {
		return `
            <canvas></canvas>
        `;
	}

	updateFrame() {
		super.updateFrame();
	}

	init() {

		console.log(this.parameters)

		super.init();
		this.data = [];
		
		this.canvasElement = this.querySelector('canvas');
		this.canvas = this.canvasElement.getContext("webgl");
		
		console.log(this.parameters.height)
		console.log(this.canvas.width)
		console.log(this.canvasElement.width)

		this.models_loaded = false;

		// Scene
		this.scene = new THREE.Scene();

		// Camera
		this.camera = new THREE.PerspectiveCamera(30, 1, 0.1, 1000);
		//this.camera.position.set(3, 0.15, 3);
		//this.cameraTarget = new THREE.Vector3(0, 2, 1);
		this.cameraTarget = new THREE.Vector3(this.parameters.camera1, this.parameters.camera2, this.parameters.camera3);
		//controls.target.set( 0, -0.2, -0.2 );

		this.camera.position.set(5, 2, 0.5);

		// Fog
		this.scene.fog = new THREE.Fog(0x72645b, 2, 15);

		// Axis
		var axisHelper = new THREE.AxisHelper(5);
		this.scene.add(axisHelper);


		// Ground
		//var plane = new THREE.Mesh(
		//     new THREE.PlaneBufferGeometry(40, 40),
		//     new THREE.MeshPhongMaterial({ color: 0x999999, specular: 0x101010 })
		// );
		// plane.rotation.x = -Math.PI / 2;
		// plane.position.y = 0;
		// this.scene.add(plane);
		// plane.receiveShadow = true;

		// Lights
		this.scene.add(new THREE.HemisphereLight(0x443333, 0x111122));
		this.addShadowedLight(1, 1, 1, 0xffffff, 1.35);
		this.addShadowedLight(0.5, 1, -1, 0xffaa00, 1);

		// renderer
		this.renderer = new THREE.WebGLRenderer({ antialias: true, clearColor: 0x335588, canvas: this.canvas.canvas });
		//this.renderer.setClearColor(this.scene.fog.color);
		this.renderer.setClearColor(0xffff00, .5);
		this.renderer.setPixelRatio(window.devicePixelRatio);
		this.renderer.setSize(this.parameters.width,  this.parameters.height);
		this.renderer.gammaInput = true;
		this.renderer.gammaOutput = true;
		this.renderer.shadowMap.enabled = true;
		this.renderer.shadowMap.cullFace = THREE.CullFaceBack;

		this.controls = new THREE.OrbitControls(this.camera);

		this.controls.enableDamping = true; // an animation loop is required when either damping or auto-rotation are enabled
		this.controls.dampingFactor = 0.3;
		this.controls.enableZoom = true;

		var test = this;
		this.controls.addEventListener('change', function () { render(test) });

		function render(o) {
			o.camera.lookAt(o.cameraTarget);
			o.renderer.render(o.scene, o.camera);
		};

		function animate(o, time) {
			requestAnimationFrame(animate.bind(null, o));
			o.controls.update();
			render(o);
		};

		animate(this, 0);

		this.canvas.clearRect(0, 0, this.width, this.height);

	}

	addShadowedLight(x, y, z, color, intensity) {

		var directionalLight = new THREE.DirectionalLight(color, intensity);
		directionalLight.position.set(x, y, z);
		this.scene.add(directionalLight);

		directionalLight.castShadow = true;

		var d = 1;
		directionalLight.shadowCameraLeft = -d;
		directionalLight.shadowCameraRight = d;
		directionalLight.shadowCameraTop = d;
		directionalLight.shadowCameraBottom = -d;

		directionalLight.shadowCameraNear = 1;
		directionalLight.shadowCameraFar = 4;

		directionalLight.shadowMapWidth = 1024;
		directionalLight.shadowMapHeight = 1024;

		directionalLight.shadowBias = -0.005;
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

	LoadModel(id, m) {
		//console.log('Loading model')

		var manager = new THREE.LoadingManager();
		manager.onProgress = function (item, loaded, total) {
			// console.log(" Progress", item, loaded, total);
		};
		manager.onLoad = function (item, loaded, total) {
			// console.log("Everything is loaded");
		};
		manager.onError = function (item, loaded, total) {
			// console.log(" Error", item, loaded, total);
		};
		var LoadModels = 0;

		const callback = function (gltf) {
			//console.log("Callback: Loading nr " + LoadModels + " ID: " + id[0][LoadModels]);
			this.ik_objects[LoadModels] = gltf.scene

			this.scene.add(gltf.scene);
			LoadModels++;
			if (LoadModels < id[0].length) // put the next load in the callback
				loader.load('/Models/glTF/' + id[0][LoadModels] + '.gltf', callback.bind(this));
		};


		// Instantiate a loader
		var loader = new THREE.GLTFLoader(manager);

		// Load a glTF resource
		loader.load('/Models/glTF/' + id[0][0] + '.gltf', callback.bind(this))
		//console.log("Loaded fine")

	}

	requestData(data_set) {
        data_set.add(this.parameters.module + "." + this.parameters.source);
        data_set.add(this.parameters.model_mat_module + "." + this.parameters.model_mat_source);
        data_set.add(this.parameters.model_id_module + "." + this.parameters.model_id_source);
	}

	update(d) {
		if (!d)
			return;
		// Model Matrices
		this.modelMatrix = d[this.parameters['model_mat_module']][this.parameters['model_mat_source']]
		// Model ID
		this.modelId = d[this.parameters['model_id_module']][this.parameters['model_id_source']]

		if (this.modelMatrix && this.modelId) {

			if (!this.models_loaded) {
				this.ik_objects = new Array(this.modelId[0].length)
				this.LoadModel(this.modelId, this.modelMatrix);
				this.models_loaded = true;

			}
			// Update position from an array 16xid
			for (i = 0; i < this.modelId[0].length; i++) {
				this.ik_objects[i].matrixAutoUpdate = false;
				this.ik_objects[i].matrix.set(this.modelMatrix[i][0], this.modelMatrix[i][1], this.modelMatrix[i][2], this.modelMatrix[i][3], this.modelMatrix[i][4], this.modelMatrix[i][5], this.modelMatrix[i][6], this.modelMatrix[i][7], this.modelMatrix[i][8], this.modelMatrix[i][9], this.modelMatrix[i][10], this.modelMatrix[i][11], this.modelMatrix[i][12], this.modelMatrix[i][13], this.modelMatrix[i][14], this.modelMatrix[i][15]);
				this.ik_objects[i].matrix = this.IkaorsToThreeBase(this.ik_objects[i].matrix);
			}
		}
		//console.log("update Done")
	};
};

webui_widgets.add('webui-widget-canvas3d', WebUIWidgetCanvas3D);
