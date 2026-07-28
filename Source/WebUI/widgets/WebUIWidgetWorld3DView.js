class WebUIWidgetWorld3DView extends WebUIWidget
{
    static template()
    {
        return [
            {'name': "WORLD 3D VIEW", 'control':'header'},
            {'name':'title', 'default':"World 3D View", 'type':'string', 'control': 'textedit'},
            {'name':'creature_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'objects_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'walls_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'whiskers_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'whisker_length_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'whisker_angle_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'world_width', 'default':300, 'type':'float', 'control': 'textedit'},
            {'name':'world_height', 'default':300, 'type':'float', 'control': 'textedit'},
            {'name':'object_height', 'default':8, 'type':'float', 'control': 'textedit'},
            {'name':'wall_height', 'default':12, 'type':'float', 'control': 'textedit'},
            {'name':'creature_height', 'default':4, 'type':'float', 'control': 'textedit'},
            {'name':'wall_depth', 'default':2, 'type':'float', 'control': 'textedit'},
            {'name':'whisker_length', 'default':10, 'type':'float', 'control': 'textedit'},
            {'name':'whisker_angle', 'default':0.7853981633974483, 'type':'float', 'control': 'textedit'},
            {'name':'show_whiskers', 'default':'yes', 'type':'bool', 'control': 'checkbox'},
            {'name':'show_grid', 'default':"yes", 'type':'bool', 'control': 'checkbox'},
            {'name':'grid_step', 'default':10, 'type':'float', 'control': 'textedit'},
            {'name':'ground_color', 'default':"#ece1c9", 'type':'string', 'control': 'textedit'},
            {'name':'camera_distance', 'default':320, 'type':'float', 'control': 'textedit'},
            {'name':'object_opacity', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'wall_opacity', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'scene_background', 'default':"#f7f5ef", 'type':'string', 'control': 'textedit'}
        ];
    }

    static html()
    {
        return `<canvas></canvas>`;
    }

    init()
    {
        this.canvasElement = this.querySelector('canvas');
        this.canvasElement.style.display = "block";
        this.canvasElement.style.width = "100%";
        this.canvasElement.style.height = "100%";
        this.canvasElement.style.touchAction = "none";

        this.creature = [];
        this.objects = [];
        this.walls = [];
        this.whiskers = [0, 0];
        this.sceneWidth = 0;
        this.sceneHeight = 0;
        this.animationFrame = null;
        this.lastCameraDistance = null;
        this.objectsSceneKey = null;
        this.wallsSceneKey = null;
        this.creatureSceneKey = null;
        this.objectsGroup = null;
        this.wallsGroup = null;
        this.creatureGroup = null;
        this.creatureObject = null;

        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(this.parameters.scene_background || "#f7f5ef");

        this.camera = new THREE.PerspectiveCamera(45, 1, 0.1, 2000);
        this.camera.position.set(160, 170, 220).setLength(Number(this.parameters.camera_distance) || 320);

        this.renderer = new THREE.WebGLRenderer({canvas: this.canvasElement, antialias: true});
        this.renderer.setPixelRatio(window.devicePixelRatio || 1);
        this.renderer.shadowMap.enabled = true;
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;

        this.controls = new THREE.OrbitControls(this.camera, this.canvasElement);
        this.controls.enableDamping = true;
        this.controls.dampingFactor = 0.08;
        this.controls.target.set(0, 0, 0);
        this.controls.update();

        this.root = new THREE.Group();
        this.scene.add(this.root);
        this.scene.add(new THREE.HemisphereLight(0xffffff, 0xb5aa92, 0.95));

        this.sunTarget = new THREE.Object3D();
        this.scene.add(this.sunTarget);

        this.sunLight = new THREE.DirectionalLight(0xffffff, 0.35);
        this.sunLight.position.set(-110, 210, 150);
        this.sunLight.castShadow = true;
        this.sunLight.target = this.sunTarget;
        this.sunLight.shadow.mapSize.width = 2048;
        this.sunLight.shadow.mapSize.height = 2048;
        this.sunLight.shadow.bias = -0.0004;
        this.sunLight.shadow.normalBias = 0.035;
        this.sunLight.shadow.radius = 3;
        this.scene.add(this.sunLight);

        this.ground = null;
        this.shadowPlane = null;
        this.grid = null;
        this.worldOutline = null;
        this.sceneKey = "";

        this.wheelHandler = (event) => {
            if(event.cancelable)
                event.preventDefault();
            event.stopPropagation();
        };
        this.addManagedListener(this.canvasElement, "wheel", this.wheelHandler, {passive: false});

        this.resizeRenderer();
        this.refreshWorldBase();
        this.updateCameraDistance();
        this.animate();
    }

    disconnectedCallback()
    {
        super.disconnectedCallback();
        if(this.animationFrame !== null)
            cancelAnimationFrame(this.animationFrame);
        this.animationFrame = null;
        if(this.controls && typeof this.controls.dispose === "function")
            this.controls.dispose();
        if(this.root)
            this.clearGroup(this.root);
        if(this.renderer && typeof this.renderer.dispose === "function")
            this.renderer.dispose();
    }

    animate()
    {
        this.animationFrame = requestAnimationFrame(() => this.animate());
        if(this.controls)
        {
            this.controls.enabled = !(typeof main !== "undefined" && main.edit_mode);
            this.controls.update();
        }
        if(this.renderer && this.scene && this.camera)
            this.renderer.render(this.scene, this.camera);
    }

    sourceNumber(sourceName, fallback)
    {
        const value = this.getSource(sourceName, undefined);
        if(Array.isArray(value))
        {
            const first = Array.isArray(value[0]) ? value[0][0] : value[0];
            const number = Number(first);
            return Number.isFinite(number) ? number : Number(fallback);
        }

        const number = Number(value);
        return Number.isFinite(number) ? number : Number(fallback);
    }

    matrixRows(value)
    {
        if(this.getMatrixRank(value) == 1)
            return [value];
        return Array.isArray(value) ? value : [];
    }

    worldWidth()
    {
        return Math.max(1, Number(this.parameters.world_width) || 300);
    }

    worldHeight()
    {
        return Math.max(1, Number(this.parameters.world_height) || 300);
    }

    worldX(x)
    {
        return Number(x) - this.worldWidth() * 0.5;
    }

    worldZ(y)
    {
        return Number(y) - this.worldHeight() * 0.5;
    }

    rowColor(row, offset, fallback)
    {
        if(row.length > offset + 2)
        {
            const components = [row[offset], row[offset + 1], row[offset + 2]].map(Number);
            if(!components.every(Number.isFinite))
                return new THREE.Color(fallback);
            const r = Math.max(0, Math.min(1, components[0]));
            const g = Math.max(0, Math.min(1, components[1]));
            const b = Math.max(0, Math.min(1, components[2]));
            return new THREE.Color(r, g, b);
        }
        return new THREE.Color(fallback);
    }

    clearGroup(group)
    {
        while(group.children.length > 0)
        {
            const child = group.children[group.children.length - 1];
            group.remove(child);
            child.traverse((node) => {
                if(node.geometry)
                    node.geometry.dispose();
                if(node.material)
                {
                    if(Array.isArray(node.material))
                        node.material.forEach((material) => material.dispose());
                    else
                        node.material.dispose();
                }
            });
        }
    }

    updateLighting(width, height)
    {
        const span = Math.max(width, height);
        this.sunTarget.position.set(0, 0, 0);
        this.sunLight.position.set(-span * 0.42, span * 0.72, span * 0.48);
        this.sunLight.shadow.camera.left = -width * 0.65;
        this.sunLight.shadow.camera.right = width * 0.65;
        this.sunLight.shadow.camera.top = height * 0.65;
        this.sunLight.shadow.camera.bottom = -height * 0.65;
        this.sunLight.shadow.camera.near = 1;
        this.sunLight.shadow.camera.far = span * 2.0;
        this.sunLight.shadow.camera.updateProjectionMatrix();
    }

    refreshWorldBase()
    {
        const width = this.worldWidth();
        const height = this.worldHeight();
        const step = Math.max(1, Number(this.parameters.grid_step) || 10);
        const groundColor = this.parameters.ground_color || "#ece1c9";
        const background = this.parameters.scene_background || "#f7f5ef";
        const key = `${width}:${height}:${step}:${this.parameters.show_grid}:${background}:${groundColor}`;
        if(this.sceneKey === key)
            return;

        this.sceneKey = key;
        this.clearGroup(this.root);
        this.objectsSceneKey = null;
        this.wallsSceneKey = null;
        this.creatureSceneKey = null;
        this.objectsGroup = null;
        this.wallsGroup = null;
        this.creatureGroup = null;
        this.creatureObject = null;
        this.scene.background = new THREE.Color(background);
        this.updateLighting(width, height);

        const groundMaterial = new THREE.MeshBasicMaterial({
            color: new THREE.Color(groundColor),
            transparent: true,
            opacity: 0.93,
            depthWrite: false,
            side: THREE.DoubleSide
        });
        this.ground = new THREE.Mesh(new THREE.PlaneGeometry(width, height), groundMaterial);
        this.ground.rotation.x = -Math.PI / 2;
        this.root.add(this.ground);

        this.shadowPlane = new THREE.Mesh(
            new THREE.PlaneGeometry(width, height),
            new THREE.ShadowMaterial({color: 0x000000, opacity: 0.04, transparent: true})
        );
        this.shadowPlane.rotation.x = -Math.PI / 2;
        this.shadowPlane.position.y = 0.03;
        this.shadowPlane.receiveShadow = true;
        this.root.add(this.shadowPlane);

        if(this.toBool(this.parameters.show_grid))
        {
            const size = Math.max(width, height);
            const divisions = Math.max(1, Math.round(size / step));
            this.grid = new THREE.GridHelper(size, divisions, 0x7f878a, 0xb6bdc0);
            this.grid.material.transparent = true;
            this.grid.material.opacity = 0.38;
            this.grid.position.y = 0.06;
            this.root.add(this.grid);
        }

        const outlineGeometry = new THREE.BufferGeometry().setFromPoints([
            new THREE.Vector3(-width * 0.5, 0.08, -height * 0.5),
            new THREE.Vector3(width * 0.5, 0.08, -height * 0.5),
            new THREE.Vector3(width * 0.5, 0.08, height * 0.5),
            new THREE.Vector3(-width * 0.5, 0.08, height * 0.5),
            new THREE.Vector3(-width * 0.5, 0.08, -height * 0.5)
        ]);
        this.worldOutline = new THREE.Line(outlineGeometry, new THREE.LineBasicMaterial({color: 0x2f3437}));
        this.root.add(this.worldOutline);

        const distance = Math.max(width, height);
        this.camera.position.set(distance * 0.55, distance * 0.58, distance * 0.75);
        this.lastCameraDistance = null;
        this.controls.target.set(0, 0, 0);
        this.controls.maxDistance = Math.max(100, distance * 2.5);
        this.controls.update();
    }

    updateCameraDistance()
    {
        const configuredDistance = Number(this.parameters.camera_distance);
        const distance = Number.isFinite(configuredDistance) && configuredDistance > 0 ? configuredDistance : 320;
        if(this.lastCameraDistance === distance)
            return;

        const direction = this.camera.position.clone().sub(this.controls.target);
        if(direction.lengthSq() === 0)
            direction.set(1, 1, 1);
        this.camera.position.copy(this.controls.target).add(direction.setLength(distance));
        this.lastCameraDistance = distance;
        this.controls.update();
    }

    opacity(value)
    {
        const number = Number(value);
        return Number.isFinite(number) ? Math.max(0, Math.min(1, number)) : 1;
    }

    addObject(row)
    {
        if(!Array.isArray(row) || row.length < 9)
            return;

        const coordinates = [row[2], row[3]].map(Number);
        if(!coordinates.every(Number.isFinite))
            return;

        const radius = Math.max(0.1, Number(row[4]) || 1);
        const solid = Number(row[9]) > 0.5;
        const objectHeight = solid ? Math.max(0.1, Number(this.parameters.object_height) || 8) : 0.36;
        const geometry = new THREE.CylinderGeometry(radius, radius, solid ? objectHeight * 2 : objectHeight, 32);
        const color = this.rowColor(row, 6, 0x8c8f92);
        const opacity = this.opacity(this.parameters.object_opacity);
        const material = new THREE.MeshStandardMaterial({
            color,
            roughness: 0.58,
            side: THREE.FrontSide,
            opacity,
            transparent: opacity < 1
        });
        const mesh = new THREE.Mesh(geometry, material);
        const x = this.worldX(coordinates[0]);
        const z = this.worldZ(coordinates[1]);
        mesh.position.set(x, solid ? objectHeight : objectHeight * 0.5, z);
        mesh.castShadow = solid;
        mesh.receiveShadow = true;
        (this.objectsGroup || this.root).add(mesh);
    }

    addWall(row)
    {
        if(!Array.isArray(row) || row.length < 11)
            return;

        const coordinates = [row[2], row[3], row[4], row[5]].map(Number);
        if(!coordinates.every(Number.isFinite))
            return;

        const x1 = this.worldX(coordinates[0]);
        const z1 = this.worldZ(coordinates[1]);
        const x2 = this.worldX(coordinates[2]);
        const z2 = this.worldZ(coordinates[3]);
        const dx = x2 - x1;
        const dz = z2 - z1;
        const length = Math.sqrt(dx * dx + dz * dz);
        if(length <= 0.0001)
            return;

        const wallHeight = Math.max(0.1, Number(this.parameters.wall_height) || 12);
        const wallDepth = Math.max(0.1, Number(row[10]) || Number(this.parameters.wall_depth) || 2);
        const opaque = Number(row[1]) > 0.5;
        const color = this.rowColor(row, 6, 0x303236);
        if(!opaque)
        {
            const dotRadius = Math.max(0.35, wallDepth * 0.45);
            const dotSpacing = Math.max(dotRadius * 2.8, wallDepth * 2.6);
            const count = Math.max(2, Math.floor(length / dotSpacing) + 1);
            const opacity = this.opacity(this.parameters.wall_opacity);
            const material = new THREE.MeshStandardMaterial({
                color,
                roughness: 0.5,
                opacity,
                transparent: opacity < 1
            });
            for(let i = 0; i < count; ++i)
            {
                const t = count === 1 ? 0.5 : i / (count - 1);
                const dot = new THREE.Mesh(new THREE.CylinderGeometry(dotRadius, dotRadius, wallHeight * 2, 12), material.clone());
                dot.position.set(x1 + dx * t, wallHeight, z1 + dz * t);
                dot.castShadow = true;
                dot.receiveShadow = true;
                (this.wallsGroup || this.root).add(dot);
            }
            material.dispose();
            return;
        }

        const opacity = this.opacity(this.parameters.wall_opacity);
        const material = new THREE.MeshStandardMaterial({
            color,
            roughness: 0.5,
            opacity,
            transparent: opacity < 1
        });
        const mesh = new THREE.Mesh(new THREE.BoxGeometry(length, wallHeight * 2, wallDepth), material);
        mesh.position.set((x1 + x2) * 0.5, wallHeight, (z1 + z2) * 0.5);
        mesh.rotation.y = -Math.atan2(dz, dx);
        mesh.castShadow = true;
        mesh.receiveShadow = true;
        (this.wallsGroup || this.root).add(mesh);
    }

    addCreature(row)
    {
        if(!Array.isArray(row) || row.length < 4)
            return;

        const coordinates = [row[0], row[1]].map(Number);
        if(!coordinates.every(Number.isFinite))
            return;

        const radius = Math.max(0.1, Number(row[2]) || 4);
        const height = Math.max(0.1, Number(this.parameters.creature_height) || 4);
        const heading = Number(row[3]) || 0;
        const color = this.rowColor(row, 4, 0x215fc7);
        const group = new THREE.Group();
        const x = this.worldX(coordinates[0]);
        const z = this.worldZ(coordinates[1]);
        group.position.set(x, 0, z);
        group.rotation.y = -heading;

        const body = new THREE.Mesh(
            new THREE.CylinderGeometry(radius, radius, height * 2, 32),
            new THREE.MeshStandardMaterial({color, roughness: 0.45})
        );
        body.position.y = height;
        body.castShadow = true;
        body.receiveShadow = true;
        group.add(body);

        if(this.toBool(this.parameters.show_whiskers))
            this.addWhiskers(group, radius, height);
        (this.creatureGroup || this.root).add(group);
        this.creatureObject = group;
    }

    addWhiskers(group, radius, height)
    {
        const length = this.sourceNumber('whisker_length_parameter', this.parameters.whisker_length);
        const angle = this.sourceNumber('whisker_angle_parameter', this.parameters.whisker_angle);
        const y = height;
        this.addWhisker(group, -angle, length, Number(this.whiskers[0]) || 0, y, radius);
        this.addWhisker(group, angle, length, Number(this.whiskers[1]) || 0, y, radius);
    }

    addWhisker(group, angle, length, signal, y, radius)
    {
        const value = Math.max(0, Math.min(1, signal));
        const start = new THREE.Vector3(radius * 0.45, y, 0);
        const end = new THREE.Vector3(
            Math.cos(angle) * Math.max(0, Number(length)),
            y,
            Math.sin(angle) * Math.max(0, Number(length))
        );
        const direction = new THREE.Vector3().subVectors(end, start);
        const whiskerLength = direction.length();
        if(whiskerLength <= 0.0001)
            return;

        const color = value > 0 ? new THREE.Color(1, 0.16 + 0.35 * (1 - value), 0.12) : new THREE.Color(0x20384d);
        const geometry = new THREE.CylinderGeometry(0.18 + 0.16 * value, 0.18 + 0.16 * value, whiskerLength, 8);
        const material = new THREE.MeshBasicMaterial({color});
        const mesh = new THREE.Mesh(geometry, material);
        mesh.position.copy(start).addScaledVector(direction, 0.5);
        mesh.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), direction.normalize());
        group.add(mesh);
    }

    ensureDynamicGroups()
    {
        if(!this.objectsGroup)
        {
            this.objectsGroup = new THREE.Group();
            this.root.add(this.objectsGroup);
        }
        if(!this.wallsGroup)
        {
            this.wallsGroup = new THREE.Group();
            this.root.add(this.wallsGroup);
        }
        if(!this.creatureGroup)
        {
            this.creatureGroup = new THREE.Group();
            this.root.add(this.creatureGroup);
        }
    }

    updateCreatureTransform()
    {
        if(!this.creatureObject || !Array.isArray(this.creature) || this.creature.length < 4)
            return;
        const coordinates = [this.creature[0], this.creature[1]].map(Number);
        if(!coordinates.every(Number.isFinite))
            return;
        const heading = Number(this.creature[3]);
        this.creatureObject.position.set(this.worldX(coordinates[0]), 0, this.worldZ(coordinates[1]));
        this.creatureObject.rotation.y = Number.isFinite(heading) ? -heading : 0;
    }

    updateObjectTransforms()
    {
        let meshIndex = 0;
        for(const row of this.objects)
        {
            if(!Array.isArray(row) || row.length < 9)
                continue;
            const coordinates = [row[2], row[3]].map(Number);
            if(!coordinates.every(Number.isFinite))
                continue;
            const mesh = this.objectsGroup.children[meshIndex++];
            if(mesh)
                mesh.position.set(this.worldX(coordinates[0]), mesh.position.y, this.worldZ(coordinates[1]));
        }
    }

    updateSceneObjects()
    {
        this.refreshWorldBase();
        this.updateCameraDistance();
        this.ensureDynamicGroups();

        const objectsSceneKey = JSON.stringify({
            appearance: this.objects.map((row) => Array.isArray(row) ? [...row.slice(0, 2), ...row.slice(4)] : row),
            objectHeight: this.parameters.object_height,
            objectOpacity: this.parameters.object_opacity
        });
        if(this.objectsSceneKey !== objectsSceneKey)
        {
            this.objectsSceneKey = objectsSceneKey;
            this.clearGroup(this.objectsGroup);
            for(const row of this.objects)
                this.addObject(row);
        }
        else
            this.updateObjectTransforms();

        const wallsSceneKey = JSON.stringify({
            walls: this.walls,
            wallHeight: this.parameters.wall_height,
            wallDepth: this.parameters.wall_depth,
            wallOpacity: this.parameters.wall_opacity
        });
        if(this.wallsSceneKey !== wallsSceneKey)
        {
            this.wallsSceneKey = wallsSceneKey;
            this.clearGroup(this.wallsGroup);
            for(const row of this.walls)
                this.addWall(row);
        }

        const creatureAppearance = Array.isArray(this.creature) ? [this.creature[2], ...this.creature.slice(4)] : [];
        const creatureSceneKey = JSON.stringify({
            appearance: creatureAppearance,
            whiskers: this.whiskers,
            creatureHeight: this.parameters.creature_height,
            showWhiskers: this.toBool(this.parameters.show_whiskers),
            whiskerLength: this.sourceNumber('whisker_length_parameter', this.parameters.whisker_length),
            whiskerAngle: this.sourceNumber('whisker_angle_parameter', this.parameters.whisker_angle)
        });
        if(this.creatureSceneKey !== creatureSceneKey)
        {
            this.creatureSceneKey = creatureSceneKey;
            this.clearGroup(this.creatureGroup);
            this.creatureObject = null;
            this.addCreature(this.creature);
        }
        else
            this.updateCreatureTransform();
    }

    resizeRenderer()
    {
        this.updateFrame();
        const width = Math.max(1, this.offsetWidth);
        const height = Math.max(1, this.offsetHeight);
        if(width === this.sceneWidth && height === this.sceneHeight)
            return;

        this.sceneWidth = width;
        this.sceneHeight = height;
        this.renderer.setSize(width, height, false);
        this.camera.aspect = width / height;
        this.camera.updateProjectionMatrix();
    }

    update(d)
    {
        this.resizeRenderer();
        const creature = this.getSource('creature_source', []);
        this.creature = this.getMatrixRank(creature) == 1 ? creature : [];
        this.objects = this.matrixRows(this.getSource('objects_source', []));
        this.walls = this.matrixRows(this.getSource('walls_source', []));
        const whiskers = this.getSource('whiskers_source', []);
        this.whiskers = this.getMatrixRank(whiskers) == 1 ? whiskers : [0, 0];
        this.updateSceneObjects();
    }
}

webui_widgets.add('webui-widget-world3dview', WebUIWidgetWorld3DView);
