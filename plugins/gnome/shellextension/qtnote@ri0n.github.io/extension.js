/*
    SPDX-License-Identifier: GPL-3.0-only
*/

import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';

import {QtNoteDBusClient} from './dbus.js';
import {QtNoteIndicator} from './indicator.js';
import {Keybindings} from './keybindings.js';

export default class QtNoteExtension extends Extension {
    enable() {
        this._dbus = new QtNoteDBusClient();
        this._settings = this.getSettings('org.gnome.shell.extensions.qtnote');
        this._keybindings = new Keybindings(this._settings, this._dbus);
        this._keybindings.enable();
        this._indicator = new QtNoteIndicator(this, this._dbus);
        Main.panel.addToStatusArea(this.uuid, this._indicator);
    }

    disable() {
        this._indicator?.destroy();
        this._indicator = null;

        this._keybindings?.disable();
        this._keybindings = null;
        this._settings = null;

        this._dbus?.destroy();
        this._dbus = null;
    }
}
