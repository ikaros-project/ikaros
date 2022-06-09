class WebUIWidgetCanvas3D extends WebUIWidget {
	static template() {
		return [
			{ 'name': "CANVAS 3D", 'control': 'header' },
			{ 'name': 'matrix', 'default': "", 'type': 'source', 'control': 'textedit' },

			{ 'name': "CONTROL", 'control': 'header' },
			{ 'name': 'show_models', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'models', 'default': "", 'type': 'string', 'control': 'textedit' },
			{ 'name': 'show_lines', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'line_color', 'default': "blue", 'type': 'string', 'control': 'textedit' },

			{ 'name': 'show_points', 'default': true, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'point_color', 'default': "black", 'type': 'string', 'control': 'textedit' },
			{ 'name': 'point_size', 'default': "0.15", 'type': 'string', 'control': 'textedit' },

			{ 'name': 'show_axis', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'show_ground_grid', 'default': false, 'type': 'bool', 'control': 'checkbox' },
			{ 'name': 'show_stats', 'default': false, 'type': 'bool', 'control': 'checkbox' },

			{ 'name': 'views', 'default': "Home", 'type': 'string', 'control': 'menu', 'values': "Home, Top, Bottom, Front, Back, Left, Right" },
			{ 'name': 'look_at_X', 'default': "0", 'type': 'float', 'control': 'textedit' },
			{ 'name': 'look_at_Y', 'default': "0.8", 'type': 'float', 'control': 'textedit' },
			{ 'name': 'look_at_Z', 'default': "0", 'type': 'float', 'control': 'textedit' },

			{ 'name': "FRAME", 'control': 'header' },
            { 'name': 'show_title', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'show_frame', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'style', 'default': "", 'type': 'string', 'control': 'textedit' },
            { 'name': 'frame-style', 'default': "", 'type': 'string', 'control': 'textedit' },
			{'name':'show_loading', 'default':false, 'type':'bool', 'control': 'checkbox'},

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

		<div id = "demo"></div>
		<canvas></canvas>
        `;
	}

	updateAll() {
		this.FixedView = true;

	}

	init() {
		this.points_loaded = false;
		this.canvasElement = this.querySelector('canvas');
		this.canvas = this.canvasElement.getContext("webgl");
		this.models_loaded = false;

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
		this.s = document.getElementById("demo");
		this.s.appendChild(this.stats.domElement);
		document.getElementById("demo").firstChild.style.position = "absolute"
		document.getElementById("demo").firstChild.style.top = "25px"
		document.getElementById("demo").firstChild.style.left = "5px"

		// Renderer
		this.renderer = new THREE.WebGLRenderer({ antialias: true, clearColor: 0x335588, canvas: this.canvas.canvas });
		this.renderer.setClearColor( 0x263238 );
		this.renderer.setPixelRatio(window.devicePixelRatio);
		this.renderer.setSize(this.parameters.width, this.parameters.height);
		this.renderer.shadowMap.enabled = true;
		//this.renderer.shadowMap.cullFace = THREE.CullFaceBack;
		this.renderer.shadowMapType = THREE.PCFSoftShadowMap

		this.controls = new THREE.OrbitControls(this.camera, this.canvas.canvas);
		this.controls.enablePan = true;
		this.controls.enableDamping = true;
		this.controls.minDistance = 0.5;
		this.controls.maxDistance = 4;
		this.controls.update();
		
		function render(o) {
			o.camera.lookAt(o.cameraTarget);
			o.renderer.render(o.scene, o.camera);
		};

		function animate(o, time) {
			requestAnimationFrame(animate.bind(null, o));
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

	update() {

		this.updateFrame();
		if (!(this.data = this.getSource('matrix'))) {
			console.log("No 3D data from ikaros")
			return;
		}
		//console.log("Formating data")
		this.mat = []
		this.vertices = []
		for (var i = 0; i < this.data.length; i++) {
			this.mat.push(new THREE.Matrix4());
			//this.mat[i].fromArray(this.data[i]) // Not the right order
			this.mat[i].set(this.data[i][0], this.data[i][1], this.data[i][2], this.data[i][3], this.data[i][4], this.data[i][5], this.data[i][6], this.data[i][7], this.data[i][8], this.data[i][9], this.data[i][10], this.data[i][11], this.data[i][12], this.data[i][13], this.data[i][14], this.data[i][15]);
			this.mat[i] = this.IkaorsToThreeBase(this.mat[i]);
			this.vertices.push(this.mat[i].elements[12], this.mat[i].elements[13], this.mat[i].elements[14])
		}
		this.nrOfModels = this.data.length
		//console.log("Formating data done")



		// Models
		if (this.toBool(this.parameters.show_models)) {
			//console.log('Models')
			this.modelNames = this.parameters.models.split(',');

			if (this.lastmodels != this.parameters.models) // Remove object if list changed
			{
				if (this.model_objects)
					for (var i = 0; i < this.nrOfModels; i++)
						this.scene.remove(this.model_objects[i]);
				this.models_loaded = false
			}
			this.lastmodels = this.parameters.models

			// Load models at first update or change of models array
			if (!this.models_loaded) {
				this.widget_loading(true)
				//console.log('Loading models')
				this.model_objects = new Array(this.nrOfModels) 
				this.LoadModel(this.model_objects, this.modelNames, this.data);
				this.models_loaded = true;
			}

			// Update position from an array 16xid
			for (var i = 0; i < this.nrOfModels; i++) {
				this.model_objects[i].visible = true;
				this.model_objects[i].matrixAutoUpdate = false;
				this.model_objects[i].matrix.copy(this.mat[i])
			}


			//console.log('Updated models')

		}
		else  // Hide 
			if (this.models_loaded)
				if (this.model_objects)
					for (var i = 0; i < this.nrOfModels; i++)
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
				//console.log("Adding poitns")
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
				this.pColors = this.parameters.point_color.toLowerCase().split(',');
				this.pSize = this.parameters.point_size.split(',');

				var positions = this.particles.geometry.attributes.position.array;
				var colors = this.particles.geometry.attributes.customColor.array;
				var sizes = this.particles.geometry.attributes.size.array;

				// Update position from an array 16xi
				var cIndex = 0;
				var pIndex = 0
				for (var i = 0; i < this.vertices.length; i++)
					positions[i] = this.vertices[i]

				for (var i = 0; i < this.data.length; i++) {
					// positions[i * 3 + 0] = this.mat[i].elements[12]
					// positions[i * 3 + 1] = this.mat[i].elements[13]
					// positions[i * 3 + 2] = this.mat[i].elements[14]

					if (cIndex >= this.pColors.length)
						cIndex = 0;
					var color = new THREE.Color(this.pColors[cIndex]);
					cIndex++;
					colors[i * 3 + 0] = color.r
					colors[i * 3 + 1] = color.g
					colors[i * 3 + 2] = color.b

					if (pIndex >= this.pSize.length)
						pIndex = 0;
					sizes[i] = this.pSize[pIndex]
					pIndex++;
				}
				this.particles.geometry.attributes.position.needsUpdate = true;
				this.particles.geometry.attributes.customColor.needsUpdate = true;
				this.particles.geometry.attributes.size.needsUpdate = true;
			}
			this.particles.visible = true;
		}
		else  // Hide 
		{
			//console.log("hiding points")
			if (this.points_loaded)
				this.particles.visible = false;
		}


		// Line
		if (this.toBool(this.parameters.show_lines)) {
			this.l = this.parameters.line.split(',');
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
				geometry.setIndex(new THREE.BufferAttribute(new Uint16Array(this.l), 1));


				this.linesObject = new THREE.LineSegments(geometry, material);
				this.scene.add(this.linesObject);
			}
			else {
				// Calculate point color
				this.lColors = this.parameters.line_color.toLowerCase().split(',');
				var colors = this.linesObject.geometry.attributes.color.array;

				// Update position from an array 16xi
				var cIndex = 0;

				for (var i = 0; i < this.data.length; i++) {
					if (cIndex >= this.lColors.length)
						cIndex = 0;
					var color = new THREE.Color(this.lColors[cIndex]);
					cIndex++;
					colors[i * 3 + 0] = color.r
					colors[i * 3 + 1] = color.g
					colors[i * 3 + 2] = color.b
				}
				this.linesObject.geometry.attributes.color.needsUpdate = true;

				var positions = this.linesObject.geometry.attributes.position.array;

				var geometry = this.linesObject.geometry;
				geometry.setIndex(new THREE.BufferAttribute(new Uint16Array(this.l), 1));

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
			if (this.lines_loaded)
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

		// If we change the area in edit mode. Only work in perspective camera
		this.renderer.setSize(this.parameters.width, this.parameters.height);
		this.camera.aspect = this.parameters.width / this.parameters.height;

		// update the camera position. This should only be made if parameter is updated.
		//this.camera.position.set(this.parameters.camera_X,this.parameters.camera_Y,this.parameters.camera_Z);

		// update the camera look at
		this.cameraTarget.set(this.parameters.look_at_X, this.parameters.look_at_Y, this.parameters.look_at_Z);
		this.camera.updateProjectionMatrix();

		if (this.toBool(this.FixedView)) {
			switch (this.parameters.views) {
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
		}
		// var t0 = performance.now();
		// var t1 = performance.now();
		// console.log("Update took " + (t1 - t0) + " milliseconds.");



		this.FixedView = false;




	};
	
};



webui_widgets.add('webui-widget-canvas3d', WebUIWidgetCanvas3D);
