var threescene = null;

function Ikaros3D(p)
{
    // Model Matrix
    this.model_mat_module = p.model_mat_module
    this.model_mat_source = p.model_mat_source
    usesData(this.model_mat_module, this.model_mat_source);

    // Model ID
    this.model_id_module = p.model_id_module;
    this.model_id_source = p.model_id_source;
    usesData(this.model_id_module, this.model_id_source);

    // Zahra lines
    this.p2p_module = p.p2p_module;
    this.p2p_source = p.p2p_source;
    usesData(this.p2p_module, this.p2p_source);
    var DrawedLines = 0;


    //this.cam_type = (p.cam_type ? p.cam_type : 0);
    p.opaque = "no";
    
    this.obj = 	new WebUICanvas(this, p, "webgl");

    this.top = p.x;
    this.left = p.y;
    
    if ( ! Detector.webgl ) Detector.addGetWebGLMessage();
    
    var container, stats;
    var camera, cameraTarget, scene, renderer;
    
    var ik_objects = [];
    var ik_p2p_lines = [];
    var that = this;

    var models_loaded = false;
    var p2p_lines_loaded = false;
    init();
    animate();

    function init() 
    {
        container = document.createElement( 'div' );
        document.body.appendChild( container );

        // Scene
        scene = new THREE.Scene();

        // Camera
        camera = new THREE.PerspectiveCamera( 85, this.width/this.height, 0.1, 100 );
        camera.position.set( 3, 0.15, 3 );
        cameraTarget = new THREE.Vector3( 0, -0.25, 0 );

        // Fog
        scene.fog = new THREE.Fog( 0x72645b, 2, 15 );
        
        // Axis
        //var axisHelper = new THREE.AxisHelper( 5 );
        //scene.add( axisHelper );


        // Ground
        var plane = new THREE.Mesh(
                                   new THREE.PlaneBufferGeometry( 40, 40 ),
                                   new THREE.MeshPhongMaterial( { color: 0x999999, specular: 0x101010 } )
                                   );
        plane.rotation.x = -Math.PI/2;
        plane.position.y = 0;
        scene.add( plane ); 
        plane.receiveShadow = true;
        
        // Lights
        scene.add( new THREE.HemisphereLight( 0x443333, 0x111122 ) );
        addShadowedLight( 1, 1, 1, 0xffffff, 1.35 );
        addShadowedLight( 0.5, 1, -1, 0xffaa00, 1 );
        
        // renderer
        renderer = new THREE.WebGLRenderer({ antialias: true, clearColor: 0x335588, canvas: that.canvas} ); 
        renderer.setClearColor( scene.fog.color );
        renderer.setPixelRatio( window.devicePixelRatio );
        renderer.setSize(this.width, this.height);
        renderer.gammaInput = true;
        renderer.gammaOutput = true;
        renderer.shadowMap.enabled = true;
        renderer.shadowMap.cullFace = THREE.CullFaceBack;

        // stats
        
        stats = new Stats();
        stats.domElement.style.position = 'absolute';
        stats.domElement.style.top = that.top+20;
        stats.domElement.style.left = that.left+20;
        container.appendChild( stats.domElement );
        
        
        controls = new THREE.TrackballControls( camera,that.canvas);
				controls.rotateSpeed = 1.0;
				controls.zoomSpeed = 1.2;
				controls.panSpeed = 0.8;
				controls.noZoom = false;
				controls.noPan = false;
				controls.staticMoving = true;
				controls.dynamicDampingFactor = 0.3;
				controls.keys = [ 65, 83, 68 ];
				controls.addEventListener( 'change', render );

		window.addEventListener( 'resize', onWindowResize, false );
    }
    function addShadowedLight( x, y, z, color, intensity ) {
        
        var directionalLight = new THREE.DirectionalLight( color, intensity );
        directionalLight.position.set( x, y, z );
        scene.add( directionalLight );
        
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
    
    function onWindowResize() {
        camera.aspect = that.width  / that.height;
        camera.updateProjectionMatrix();
        renderer.setSize( that.width, that.height);
		controls.handleResize();
		render();
    }
   
    function animate() {
        requestAnimationFrame( animate ); 
        stats.update();
		controls.update();
        render();
    }
    
    function render() {
        camera.lookAt( cameraTarget ); 
        renderer.render( scene, camera );
    }

    function LoadSTL(id,m){
        var manager = new THREE.LoadingManager();
        manager.onProgress = function ( item, loaded, total ) 
        {
            //console.log(" Progress" , item, loaded, total );

        };
        manager.onLoad = function ( item, loaded, total ) 
        {
            console.log("Everything is loaded");
        };
            manager.onError = function ( item, loaded, total ) 
        {
            console.log(" Error" , item, loaded, total );
        };

        var callback = function ( geometry ) 
        { 
            //console.log("Callback: Loading nr " + LoadModels + " ID: " +id[0][LoadModels]);
            //console.log("Callback: Loading" + '/Classes/Models/stl/' + id[0][LoadModels] + '.stl');
            if (geometry.hasColors) 
            {
                //console.log("COLOR STL");				
	            material = new THREE.MeshPhongMaterial({ opacity: geometry.alpha, vertexColors: THREE.VertexColors });
            }
            else
            {
                var material = new THREE.MeshPhongMaterial( { color: 0xff5533, specular: 0x111111, shininess: 200 } );  
            } 
            obj = new THREE.Mesh( geometry, material );
            // Array
            //obj.matrix.set(m[0][0+(i*16)], m[0][1+(i*16)], m[0][2+(i*16)], m[0][3+(i*16)], m[0][4+(i*16)], m[0][5+(i*16)], m[0][6+(i*16)], m[0][7+(i*16)], m[0][8+(i*16)], m[0][9+(i*16)], m[0][10+(i*16)], m[0][11+(i*16)], m[0][12+(i*16)], m[0][13+(i*16)], m[0][14+(i*16)], m[0][15+(i*16)]);
            //obj.matrix.set(m[i][0], m[i][1], m[i][2], m[i][3], m[i][4], m[i][5], m[i][6], m[i][7], m[i][8], m[i][9], m[i][10], m[i][11], m[0][12], m[i][13], m[i][14], m[i][15]);
            obj.castShadow = true;
            obj.receiveShadow = true;
            ik_objects[LoadModels] = obj;
            scene.add(obj);
            LoadModels++;

            if (LoadModels < id[0].length)  // put the next load in the callback
                loader.load('/Classes/Models/stl/' + id[0][LoadModels] + '.stl', callback ) ;
        };

        LoadModels = 0;
        var loader = new THREE.STLLoader(manager);
        loader.load('/Classes/Models/stl/' + id[0][i] + '.stl', callback) 
    }

    function IkaorsToThreeBase(m)
    {
        var r1 = new THREE.Matrix4();
        r1.makeRotationY(-Math.PI/2); 
        var r2 = new THREE.Matrix4();
        r2.makeRotationZ(-Math.PI/2);           
        var r = new THREE.Matrix4();  
        var ret = new THREE.Matrix4();  
        r.multiplyMatrices( r2, r1);
        ret.multiplyMatrices( r, m);
        return ret
    }

    this.Update = function (data) 
    {
        // Models
        var m = data[this.model_mat_module];
        if (m)
            m = m[this.model_mat_source]
       
        var id = data[this.model_id_module];
        if (id)
            id = id[this.model_id_source]
        
        if (m && id)
        {
           if (!models_loaded)
        {
            // Load STL
            LoadSTL(id,m)
            models_loaded = true;
        }
        // Update position from an array 16xid
        for (i = 0; i < id[0].length; i++)
        {
            ik_objects[i].matrixAutoUpdate = false;
            ik_objects[i].matrix.set(m[i][0], m[i][1], m[i][2], m[i][3], m[i][4], m[i][5], m[i][6], m[i][7], m[i][8], m[i][9], m[i][10], m[i][11], m[i][12], m[i][13], m[i][14], m[i][15]);
            ik_objects[i].matrix = IkaorsToThreeBase(ik_objects[i].matrix); 
        }  
        }
        
        // Lines between coordinates
        var p2p = data[this.p2p_module];
        if (p2p)
            p2p = p2p[this.p2p_source]

        if (p2p)
           if (!p2p_lines_loaded)
            {
                for (i = 0; i < p2p.length; i++)
                {
                    var material = new THREE.LineBasicMaterial( { color: 0x0000ff} );
                    var geometry = new THREE.Geometry();
                    geometry.vertices.push(new THREE.Vector3( 0, 0, 0) );
                    geometry.vertices.push(new THREE.Vector3( 0, 0, 0) );
                    var line = new THREE.Line( geometry, material );
                    ik_p2p_lines[DrawedLines] = line;
                    scene.add(line);
                    DrawedLines++;

                }
            p2p_lines_loaded = true;
            }
        
        // Update position from an array 16xid
        for (i = 0; i < p2p.length; i++)
        {
            var m1 = new THREE.Matrix4();
            m1.set(p2p[i][0], p2p[i][1], p2p[i][2], p2p[i][3], p2p[i][4], p2p[i][5], p2p[i][6], p2p[i][7], p2p[i][8], p2p[i][9], p2p[i][10], p2p[i][11], p2p[i][12], p2p[i][13], p2p[i][14], p2p[i][15]);
            m1 = IkaorsToThreeBase(m1);  

            var m2 = new THREE.Matrix4();
            m2.set(p2p[i][16], p2p[i][17], p2p[i][18], p2p[i][19], p2p[i][20], p2p[i][21], p2p[i][22], p2p[i][23], p2p[i][24], p2p[i][25], p2p[i][26], p2p[i][27], p2p[i][28], p2p[i][29], p2p[i][30], p2p[i][31])
            m2 = IkaorsToThreeBase(m2);  

            // Point 1
            ik_p2p_lines[i].geometry.vertices[0].setFromMatrixPosition(m1) 
            // Point 2
            ik_p2p_lines[i].geometry.vertices[1].setFromMatrixPosition(m2)
            ik_p2p_lines[i].geometry.verticesNeedUpdate = true;
        }  
    }
}