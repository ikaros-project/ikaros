class WebUIWidgetJoystick extends WebUIWidgetControl {
    static template() {
        return [
            { name: "JOYSTICK", control: "header" },
            { name: "title", default: "Joystick", type: "string", control: "textedit" },

            { name: "CONTROL", control: "header" },
            { name: "x_parameter", default: "", type: "source", control: "textedit" },
            { name: "y_parameter", default: "", type: "source", control: "textedit" },
            { name: "enabled_source", default: "", type: "source", control: "textedit" },
            { name: "select_x", default: 0, type: "int", control: "textedit" },
            { name: "select_y", default: "", type: "string", control: "textedit" },

            { name: "BEHAVIOR", control: "header" },
            { name: "return_to_center", default: "yes", type: "bool", control: "checkbox" },
            { name: "range", default: "bipolar", type: "string", control: "menu", options: "bipolar,unipolar" }
        ];
    }

    static html() {
        return `
            <div class="joystick">
                <div class="joystick-pad">
                    <div class="joystick-axis joystick-axis-x"></div>
                    <div class="joystick-axis joystick-axis-y"></div>
                    <div class="joystick-thumb"></div>
                </div>
            </div>
        `;
    }

    disconnectedCallback() {
        this._unbindDocumentDragHandlers();
        super.disconnectedCallback();
        this._joystickHandlersBound = false;
    }

    _getPad() {
        return this.querySelector(".joystick-pad");
    }

    _getThumb() {
        return this.querySelector(".joystick-thumb");
    }

    _isUnipolar() {
        return String(this.parameters.range || "").toLowerCase() === "unipolar";
    }

    _centerPosition() {
        return { x: 0.5, y: 0.5 };
    }

    _clamp01(value) {
        const number = Number(value);
        return Number.isFinite(number) ? Math.max(0, Math.min(1, number)) : 0.5;
    }

    _normalizedToValue(value) {
        const clamped = this._clamp01(value);
        if (this._isUnipolar()) {
            return clamped;
        }
        return clamped * 2 - 1;
    }

    _valueToNormalized(value) {
        const number = Number(value);
        if (!Number.isFinite(number)) {
            return 0.5;
        }
        if (this._isUnipolar()) {
            return this._clamp01(number);
        }
        return this._clamp01((number + 1) / 2);
    }

    _formatValue(value) {
        const number = Number(value);
        return (Number.isFinite(number) ? number : 0).toFixed(4).replace(/\.?0+$/, "");
    }

    _sendAxisValue(parameter, value) {
        this.sendIndexedControlChange(parameter, this._formatValue(value));
    }

    _sendPosition(position) {
        this._sendAxisValue(this.parameters.x_parameter, this._normalizedToValue(position.x));
        this._sendAxisValue(this.parameters.y_parameter, this._normalizedToValue(position.y));
    }

    _setThumbPosition(position) {
        const thumb = this._getThumb();
        const pad = this._getPad();
        if (!thumb || !pad) {
            return;
        }

        const x = this._clamp01(position.x);
        const y = this._clamp01(position.y);
        const maxLeft = Math.max(0, pad.clientWidth - thumb.offsetWidth);
        const maxTop = Math.max(0, pad.clientHeight - thumb.offsetHeight);
        this.position = { x, y };
        thumb.style.left = `${x * maxLeft}px`;
        thumb.style.top = `${y * maxTop}px`;
    }

    _positionFromPointer(event) {
        const pad = this._getPad();
        if (!pad) {
            return this.position || this._centerPosition();
        }

        const rect = pad.getBoundingClientRect();
        const thumb = this._getThumb();
        const thumbWidth = thumb?.offsetWidth || 0;
        const thumbHeight = thumb?.offsetHeight || 0;
        const width = Math.max(1, rect.width - thumbWidth);
        const height = Math.max(1, rect.height - thumbHeight);

        return {
            x: this._clamp01((event.clientX - rect.left - thumbWidth / 2) / width),
            y: this._clamp01((event.clientY - rect.top - thumbHeight / 2) / height)
        };
    }

    _dragTo(event) {
        const position = this._positionFromPointer(event);
        this._setThumbPosition(position);
        this._sendPosition(position);
    }

    _unbindDocumentDragHandlers() {
        for (const remove of this._dragListenerRemovers || [])
            remove();
        this._dragListenerRemovers = [];
        this._dragMoveHandler = null;
        this._dragEndHandler = null;
    }

    _eventPoint(event) {
        if (event.touches && event.touches.length > 0) {
            return event.touches[0];
        }
        if (event.changedTouches && event.changedTouches.length > 0) {
            return event.changedTouches[0];
        }
        return event;
    }

    _startDrag(event) {
        if (main.edit_mode || !this.isControlEnabled()) {
            return;
        }

        event.preventDefault();
        event.stopPropagation();
        this._unbindDocumentDragHandlers();
        this.is_active = true;

        this._dragTo(this._eventPoint(event));

        this._dragMoveHandler = (moveEvent) => {
            moveEvent.preventDefault();
            this._dragTo(this._eventPoint(moveEvent));
        };

        this._dragEndHandler = (endEvent) => {
            endEvent.preventDefault();
            endEvent.stopPropagation();
            this._unbindDocumentDragHandlers();

            if (this.toBool(this.parameters.return_to_center)) {
                const center = this._centerPosition();
                this._setThumbPosition(center);
                this._sendPosition(center);
            }

            this.is_active = false;
        };

        this._dragListenerRemovers = [
            this.addManagedListener(document, "mousemove", this._dragMoveHandler, true),
            this.addManagedListener(document, "mouseup", this._dragEndHandler, true),
            this.addManagedListener(document, "touchmove", this._dragMoveHandler, { capture: true, passive: false }),
            this.addManagedListener(document, "touchend", this._dragEndHandler, true),
            this.addManagedListener(document, "touchcancel", this._dragEndHandler, true)
        ];
    }

    requestData(data_set) {
        this.addSource(data_set, this.parameters.x_parameter);
        this.addSource(data_set, this.parameters.y_parameter);
        if (this.parameters.enabled_source) {
            this.addSource(data_set, this.parameters.enabled_source);
        }
    }

    updateAll() {
        super.updateAll();

        const pad = this._getPad();
        if (pad && !this._joystickHandlersBound) {
            this.addManagedListener(pad, "mousedown", (event) => this._startDrag(event), false);
            this.addManagedListener(pad, "touchstart", (event) => this._startDrag(event), { passive: false });
            this._joystickHandlersBound = true;
        }

        if (!this.position) {
            this._setThumbPosition(this._centerPosition());
        }
    }

    update() {
        if (this.is_active) {
            return;
        }

        this.syncControlEnabledState();
        const hasXParameter = !!this.parameters.x_parameter;
        const hasYParameter = !!this.parameters.y_parameter;
        const x = this.getSelectedSourceValue("x_parameter");
        const y = this.getSelectedSourceValue("y_parameter");

        this._setThumbPosition({
            x: hasXParameter && x !== undefined ? this._valueToNormalized(x) : 0.5,
            y: hasYParameter && y !== undefined ? this._valueToNormalized(y) : 0.5
        });
    }
}

webui_widgets.add("webui-widget-joystick", WebUIWidgetJoystick);
