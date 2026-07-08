/*
    SPDX-License-Identifier: GPL-3.0-only
*/

import Gio from 'gi://Gio';
import GLib from 'gi://GLib';

const BUS_NAME = 'com.github.ri0n.QtNote';
const OBJECT_PATH = '/QtNote';

const QTNOTE_IFACE_XML = `
<node>
  <interface name="com.github.ri0n.QtNote">
    <method name="notesJson">
      <arg type="i" name="offset" direction="in"/>
      <arg type="i" name="limit" direction="in"/>
      <arg type="s" name="query" direction="in"/>
      <arg type="s" name="response" direction="out"/>
    </method>
    <method name="globalShortcutsJson">
      <arg type="s" name="response" direction="out"/>
    </method>
    <method name="openNote">
      <arg type="s" name="storageId" direction="in"/>
      <arg type="s" name="noteId" direction="in"/>
    </method>
    <method name="createNote"/>
    <method name="activateGlobalShortcut">
      <arg type="s" name="id" direction="in"/>
    </method>
    <method name="showNoteManager"/>
    <method name="showOptions"/>
    <method name="showAbout"/>
    <method name="quit"/>
    <signal name="notesChanged"/>
    <signal name="globalShortcutsChanged"/>
  </interface>
</node>`;

const QtNoteInterfaceInfo = Gio.DBusNodeInfo.new_for_xml(QTNOTE_IFACE_XML).interfaces[0];

export class QtNoteDBusClient {
    constructor() {
        this._proxy = Gio.DBusProxy.new_for_bus_sync(
            Gio.BusType.SESSION,
            Gio.DBusProxyFlags.DO_NOT_AUTO_START_AT_CONSTRUCTION,
            QtNoteInterfaceInfo,
            BUS_NAME,
            OBJECT_PATH,
            QtNoteInterfaceInfo.name,
            null);
        this._signalIds = [];
    }

    destroy() {
        for (const id of this._signalIds)
            this._proxy.disconnectSignal(id);
        this._signalIds = [];
        this._proxy = null;
    }

    onNotesChanged(callback) {
        const id = this._proxy.connectSignal('notesChanged', callback);
        this._signalIds.push(id);
    }

    onGlobalShortcutsChanged(callback) {
        const id = this._proxy.connectSignal('globalShortcutsChanged', callback);
        this._signalIds.push(id);
    }

    notesJson(offset, limit, query) {
        return new Promise((resolve, reject) => {
            this._proxy.call(
                'notesJson',
                new GLib.Variant('(iis)', [offset, limit, query]),
                Gio.DBusCallFlags.NONE,
                -1,
                null,
                (proxy, result) => {
                    try {
                        resolve(proxy.call_finish(result).deepUnpack()[0]);
                    } catch (error) {
                        reject(error);
                    }
                });
        });
    }

    globalShortcutsJson() {
        return new Promise((resolve, reject) => {
            this._proxy.call(
                'globalShortcutsJson',
                null,
                Gio.DBusCallFlags.NONE,
                -1,
                null,
                (proxy, result) => {
                    try {
                        resolve(proxy.call_finish(result).deepUnpack()[0]);
                    } catch (error) {
                        reject(error);
                    }
                });
        });
    }

    _call(method, parameters = null) {
        this._proxy.call(
            method,
            parameters,
            Gio.DBusCallFlags.NONE,
            -1,
            null,
            (proxy, result) => {
                try {
                    proxy.call_finish(result);
                } catch (error) {
                    logError(error, `Failed to call QtNote.${method}`);
                    return;
                }
            });
    }

    openNote(storageId, noteId) {
        this._call('openNote', new GLib.Variant('(ss)', [storageId, noteId]));
    }

    createNote() {
        this._call('createNote');
    }

    activateGlobalShortcut(id) {
        this._call('activateGlobalShortcut', new GLib.Variant('(s)', [id]));
    }

    showNoteManager() {
        this._call('showNoteManager');
    }

    showOptions() {
        this._call('showOptions');
    }

    showAbout() {
        this._call('showAbout');
    }

    quit() {
        this._call('quit');
    }
}
