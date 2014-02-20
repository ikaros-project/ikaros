function add_cube(scene, x, y, r, a, b, c)
{
	var modifier = new THREE.SubdivisionModifier(1);
    //	modifier.modify(geometry);
    
	var geometry = new THREE.CubeGeometry(a, b, c, 4, 4, 4);
	var material = new THREE.MeshLambertMaterial({color: 0xffffff, wireframe: false});
	var cube = new THREE.Mesh(geometry, material);
    
	cube.position.set(x, y, c/2); // or a or b
	cube.rotation.z = r;
	cube.castShadow = true;
	cube.receiveShadow = true;
	scene.add(cube);
    
	var cube_reflection = new THREE.Mesh(geometry, material);
	cube_reflection.position.set(x, y, -c/2-1);
	cube_reflection.rotation.z = r;
	cube_reflection.rotation.set(-Math.PI, -r, 0);
	scene.add(cube_reflection);
}

var threescene = null;

function GL3D(p)
{
    var that = this;

    this.robot_module = p.robot_module;
    this.robot_location = p.robot_location;
    
    this.cam_type = (p.cam_type ? p.cam_type : 0);

    this.obj = 	new WebUICanvas(this, p, "webgl");
    
	usesData(this.module, this.source);
	usesData(this.robot_module, this.robot_location);
    
    this.hilite_module = p.hilite_module;
    this.hilite_source = p.hilite_source;

    if(this.hilite_module)
        usesData(this.hilite_module, this.hilite_source);

    this.cubes = [];
    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera((this.cam_type ? 65 : 28), this.width/this.height, 0.1, 8000);
    this.scene.add(this.camera);
    
    this.renderer = new THREE.WebGLRenderer({ clearColor: 0x335588, canvas: this.canvas} );
    this.renderer.setSize(this.width, this.height);
    this.obj.bg.style.backgroundColor = "rgba(0,0,0,0)"
 	this.renderer.shadowMapEnabled = true;

	var floorTexture = new THREE.ImageUtils.loadTexture("/Classes/Models/floor_8.png");
	floorTexture.wrapS = floorTexture.wrapT = THREE.RepeatWrapping;
	floorTexture.repeat.set(1, 1);

	var floorMaterial = new THREE.MeshLambertMaterial( {map:floorTexture, transparent: true, opacity: 0.85 } );
    var floorGeometry = new THREE.PlaneGeometry(1700, 1200, 100, 100);
	var floor = new THREE.Mesh(floorGeometry, floorMaterial);
    
	floor.receiveShadow = true;
	this.scene.add(floor);
	
	// Add borders
    
	add_cube(this.scene, 0, 600, 0, 1720, 20, 20);
//	add_cube(this.scene, 0, -600, 0, 1720, 20, 20);
	add_cube(this.scene, 850, 0, 0, 20, 1220, 20);
	add_cube(this.scene, -850, 0, 0, 20, 1220, 20);
    
    // Front
    
	var geometry = new THREE.CubeGeometry(1720, 20, 200, 4, 4, 4);
	var material = new THREE.MeshLambertMaterial({color: 0xffffff, wireframe: false});
	var cube = new THREE.Mesh(geometry, material);
    
	cube.position.set(0, -602, -80);
	cube.castShadow = true;
	cube.receiveShadow = true;
	this.scene.add(cube);
	
	// Add cubes
    
	var size = 36;
	
	for(var i=0; i<50; i++)
	{
		var x = -300+100*(i%10);
		var y = -300+100*(i/10);
		var r = Math.PI*Math.random();		
		var c = [0xffffff, 0xffffff, 0x6666ff, 0xff6666, 0x66ff66][Math.round(i/10-0.5) % 5];
        
		var geometry = new THREE.CubeGeometry(size, size, size, 4, 4, 4);
		
		var modifier = new THREE.SubdivisionModifier(2);
		modifier.modify(geometry);
		
		var material = new THREE.MeshLambertMaterial({color: c, wireframe: false});
		var cube = new THREE.Mesh(geometry, material);
		cube.position.set(x, y, size/2);
		cube.rotation.z = r;
		cube.castShadow = true;
		cube.receiveShadow = true;
		cube.rotate = 0;
		cube.velocity = 0;
		this.scene.add(cube);
		
		var cube_reflection = new THREE.Mesh(geometry, material);
		cube_reflection.position.set(x, y, -size/2-1);
		cube_reflection.rotation.z = r;
		cube_reflection.rotation.set(-Math.PI/2, -r, 0);
		this.scene.add(cube_reflection);
		
		this.cubes.push([cube,cube_reflection]);
	}
    
    // Add hilite
    
    var g = new THREE.CubeGeometry(size, size, size);
    var m = new THREE.MeshBasicMaterial( {color: 0xffff00, wireframe: true} );
    this.hilite = new THREE.Mesh(g, m);
    this.hilite.position.set(-99999, -99999, size/2+1);
    this.hilite.rotation.z = 0;
    this.hilite.castShadow = false;
    this.hilite.receiveShadow = false;
    this.hilite.rotate = 0;
    this.hilite.velocity = 0;
    this.scene.add(this.hilite);
    
    
    // Add robot
    
    var loader = new THREE.JSONLoader(true);
    loader.onLoadProgress = function () { window.console.log("Loading"); };
    loader.load("/Classes/Models/robot.js", addRobot);

    // Add light
/*
    this.pointLight = new THREE.PointLight(0xFF0000);
    this.pointLight.intensity = 0.9;
    this.pointLight.position.x = 0;
    this.pointLight.position.y = 0;
    this.pointLight.position.z = 5;
    this.pointLight.distance = 100;
    this.scene.add(this.pointLight);
*/    
    
    // Key light
    
    var spotlight = new THREE.SpotLight(0xffffff);
    spotlight.position.set(-600*1.5, -1200*1.5, 1000*1.5);
    spotlight.shadowCameraVisible = false;
    spotlight.shadowDarkness = 0.15;
    spotlight.shadowMapWidth = 2048;
    spotlight.shadowMapHeight = 2048;
    spotlight.intensity = 1.0;
    spotlight.castShadow = true;
    this.scene.add(spotlight);
    
    // Fill light
    
    var spotlight = new THREE.SpotLight(0xffffff);
    spotlight.position.set(1200*2, -600*2, 300*2);
    spotlight.shadowCameraVisible = false;
    spotlight.shadowDarkness = 0.025;
    spotlight.shadowMapWidth = 2048;
    spotlight.shadowMapHeight = 2048;
    spotlight.intensity = 0.5;
    spotlight.castShadow = true;
    this.scene.add(spotlight);
    
    // Back light
    
    var spotlight = new THREE.SpotLight(0xffffff);
    spotlight.position.set(0,1000,200);
    spotlight.shadowCameraVisible = false;
    spotlight.shadowDarkness = 0.05;
    spotlight.shadowMapWidth = 2048;
    spotlight.shadowMapHeight = 2048;
    spotlight.intensity = 0.012;
    spotlight.castShadow = true;
    this.scene.add(spotlight);
    
    
    this.camera.position.set(0, -2000, 2300);
    this.camera.lookAt(this.scene.position);

    this.renderer.render(this.scene, this.camera);
    
    function addRobot(geometry)
    {
            var material = new THREE.MeshFaceMaterial();
    //    var material = new THREE.MeshLambertMaterial();
    //    var material = new THREE.MeshPhongMaterial();
    //     var material = new THREE.MeshBasicMaterial();
    
        that.robot = new THREE.Mesh(geometry, material);
        that.robot.castShadow = true;
        that.robot.receiveShadow = true;        
        that.scene.add(that.robot);
        
        that.robot_reflection = new THREE.Mesh(geometry, material);
        that.robot_reflection.rotation.x = -Math.PI/2;
        that.scene.add(that.robot_reflection);
    }
}



GL3D.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    var p = data[this.robot_module];
    if(!p) return;
    p = p[this.robot_location]
    if(!p) return;

    var h = data[this.hilite_module];
    if(h)
        h = h[this.hilite_source];

    for(var i=0; i<50; i++) // x = -99999 for no object
    {
        this.cubes[i][0].position.x = d[i][0]-850;
        this.cubes[i][0].position.y = d[i][1]-600;
        this.cubes[i][0].position.z = d[i][2]+18;
        this.cubes[i][0].rotation.z = d[i][3];
        this.cubes[i][1].position.x = this.cubes[i][0].position.x;
        this.cubes[i][1].position.y = this.cubes[i][0].position.y;
        this.cubes[i][1].rotation.y = -this.cubes[i][0].rotation.z;
    }
    
    if(h)
    {
        this.hilite.position.x = h[0][0]-850;
        this.hilite.position.y = h[0][1]-600;
        this.hilite.position.z = h[0][2]+1+18;
        this.hilite.rotation.z = 0*h[0][3];
    }
    
    this.robot.position.x = p[0][0]-850;
    this.robot.position.y = p[0][1]-600;
    this.robot.rotation.z = p[0][3];

    this.robot_reflection.position.x = this.robot.position.x;
    this.robot_reflection.position.y = this.robot.position.y;
    
    this.robot_reflection.rotation.set(0, Math.PI, Math.PI-this.robot.rotation.z);

    if(this.cam_type == 1)
    {
        var dx = Math.cos(p[0][3]);
        var dy = Math.sin(p[0][3]);
        this.camera.up = new THREE.Vector3(0,0,1);
        this.camera.position.set(this.robot.position.x+110*dx, this.robot.position.y+110*dy, 200);
        this.camera.lookAt({ x: this.robot.position.x+300*dx, y: this.robot.position.y+300*dy, z:0});
    }

    else if(this.cam_type == 2)
    {
        var dx = Math.cos(p[0][3]);
        var dy = Math.sin(p[0][3]);
        this.camera.up = new THREE.Vector3(0,0,1);
        this.camera.position.set(this.robot.position.x-110*dx, this.robot.position.y-110*dy, 650);
        this.camera.lookAt({ x: this.robot.position.x+400*dx, y: this.robot.position.y+400*dy, z:0});
    }

    this.renderer.render(this.scene, this.camera);
}

