const Applet = imports.ui.applet;
const Clutter = imports.gi.Clutter;
const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;
const Lang = imports.lang;
const Mainloop = imports.mainloop;
const Pango = imports.gi.Pango;
const PopupMenu = imports.ui.popupMenu;
const St = imports.gi.St;

const BUS_NAME = 'com.github.ri0n.QtNote';
const OBJECT_PATH = '/QtNote';
const PAGE_SIZE = 50;
const SEARCH_DELAY_MS = 250;

const QTNOTE_IFACE_XML = `
<node>
  <interface name="com.github.ri0n.QtNote">
    <method name="notesJson">
      <arg type="i" name="offset" direction="in"/>
      <arg type="i" name="limit" direction="in"/>
      <arg type="s" name="query" direction="in"/>
      <arg type="s" name="response" direction="out"/>
    </method>
    <method name="openNote">
      <arg type="s" name="storageId" direction="in"/>
      <arg type="s" name="noteId" direction="in"/>
    </method>
    <method name="createNote"/>
    <method name="showNoteManager"/>
    <method name="showOptions"/>
    <method name="showAbout"/>
    <method name="quit"/>
    <signal name="notesChanged"/>
  </interface>
</node>`;

const QtNoteProxy = Gio.DBusProxy.makeProxyWrapper(QTNOTE_IFACE_XML);

function _(text) {
    return text;
}

class QtNoteApplet extends Applet.IconApplet {
    constructor(metadata, orientation, panelHeight, instanceId) {
        super(orientation, panelHeight, instanceId);

        this.set_applet_icon_symbolic_name('qtnote-symbolic');
        this.set_applet_tooltip('QtNote');

        this._query = '';
        this._items = [];
        this._noteRows = [];
        this._available = true;
        this._loading = false;
        this._loadingMore = false;
        this._hasMore = false;
        this._requestSerial = 0;
        this._searchSourceId = 0;
        this._focusSourceId = 0;
        this._signalIds = [];

        this._dbus = new QtNoteProxy(
            Gio.DBus.session,
            BUS_NAME,
            OBJECT_PATH,
            Lang.bind(this, this._onProxyReady));

        this.menuManager = new PopupMenu.PopupMenuManager(this);
        this.menu = new Applet.AppletPopupMenu(this, orientation);
        this.menuManager.addMenu(this.menu);

        this._buildMenu();
        this.menu.connect('open-state-changed', Lang.bind(this, function(menu, open) {
            if (!open)
                return;

            this._refresh();
            this._scheduleSearchFocus();
        }));
    }

    on_applet_clicked() {
        this.menu.toggle();
    }

    on_applet_removed_from_panel() {
        this._destroy();
    }

    _destroy() {
        if (this._searchSourceId) {
            Mainloop.source_remove(this._searchSourceId);
            this._searchSourceId = 0;
        }
        if (this._focusSourceId) {
            Mainloop.source_remove(this._focusSourceId);
            this._focusSourceId = 0;
        }
        for (let i = 0; i < this._signalIds.length; ++i)
            this._dbus.disconnectSignal(this._signalIds[i]);
        this._signalIds = [];
    }

    _onProxyReady(proxy, error) {
        if (error) {
            global.logError(error, 'Failed to create QtNote DBus proxy');
            this._available = false;
            this._render();
            return;
        }

        this._signalIds.push(this._dbus.connectSignal('notesChanged', Lang.bind(this, this._refresh)));
        this._refresh();
    }

    _buildMenu() {
        this.menu.removeAll();

        this._rootSection = new PopupMenu.PopupMenuSection();
        this.menu.addMenuItem(this._rootSection);

        this._rootBox = new St.BoxLayout({
            vertical: true,
            style_class: 'qtnote-popup',
        });
        this._rootSection.actor.add_actor(this._rootBox);

        this._header = new St.BoxLayout({
            vertical: true,
            style_class: 'qtnote-header',
        });
        this._rootBox.add_actor(this._header);

        this._actions = new St.BoxLayout({
            style_class: 'qtnote-actions',
        });
        this._header.add_actor(this._actions);
        this._addAction(_('New Note'), 'list-add-symbolic', Lang.bind(this, function() { this._call('createNote'); }));
        this._addAction(_('Note Manager'), 'view-list-symbolic', Lang.bind(this, function() { this._call('showNoteManager'); }));
        this._addAction(_('Configure QtNote...'), 'preferences-system-symbolic', Lang.bind(this, function() { this._call('showOptions'); }));
        this._addAction(_('About QtNote'), 'help-about-symbolic', Lang.bind(this, function() { this._call('showAbout'); }));
        this._addAction(_('Close QtNote'), 'application-exit-symbolic', Lang.bind(this, function() { this._call('quit'); }));

        this._searchEntry = new St.Entry({
            name: 'menu-search-entry',
            hint_text: _('Search notes'),
            can_focus: true,
            track_hover: true,
            style_class: 'qtnote-search-entry',
        });
        this._header.add_actor(this._searchEntry);
        this._searchEntry.clutter_text.connect('text-changed', Lang.bind(this, function() {
            this._setQuery(this._searchEntry.get_text());
        }));
        this._searchEntry.clutter_text.connect('key-press-event', Lang.bind(this, this._onSearchKeyPress));

        this._headerSeparator = new St.Widget({
            style_class: 'qtnote-header-separator',
        });
        this._rootBox.add_actor(this._headerSeparator);

        this._scrollView = new St.ScrollView({
            hscrollbar_policy: St.PolicyType.NEVER,
            vscrollbar_policy: St.PolicyType.AUTOMATIC,
            overlay_scrollbars: true,
            style_class: 'qtnote-scroll-view',
        });
        this._list = new St.BoxLayout({
            vertical: true,
            style_class: 'qtnote-notes-list',
        });
        this._scrollView.add_actor(this._list);
        this._rootBox.add_actor(this._scrollView);
    }

    _addAction(label, iconName, callback) {
        let button = new St.Button({
            accessible_name: label,
            can_focus: true,
            style_class: 'notification-icon-button qtnote-action-button',
        });
        let icon = new St.Icon({
            icon_name: iconName,
            icon_size: 16,
            icon_type: St.IconType.SYMBOLIC,
            style_class: 'qtnote-action-icon',
        });
        button.set_child(icon);
        button.connect('clicked', Lang.bind(this, function() {
            callback();
            this.menu.close();
        }));
        this._actions.add_actor(button);
    }

    _scheduleSearchFocus() {
        if (this._focusSourceId)
            Mainloop.source_remove(this._focusSourceId);

        this._focusSourceId = Mainloop.timeout_add(50, Lang.bind(this, function() {
            this._focusSourceId = 0;
            if (this.menu.isOpen) {
                this._searchEntry.grab_key_focus();
                this._searchEntry.clutter_text.grab_key_focus();
            }
            return false;
        }));
    }

    _setQuery(query) {
        query = query.trim();
        if (this._query === query)
            return;

        this._query = query;
        this._loading = true;
        this._loadingMore = false;
        this._requestSerial++;
        this._render();

        if (this._searchSourceId)
            Mainloop.source_remove(this._searchSourceId);
        this._searchSourceId = Mainloop.timeout_add(SEARCH_DELAY_MS, Lang.bind(this, function() {
            this._searchSourceId = 0;
            this._refresh();
            return false;
        }));
    }

    _refresh() {
        if (this._searchSourceId) {
            Mainloop.source_remove(this._searchSourceId);
            this._searchSourceId = 0;
        }
        this._requestPage(0, false);
    }

    _loadMore() {
        if (!this._hasMore || this._loading || this._loadingMore)
            return;

        this._requestPage(this._items.length, true);
    }

    _requestPage(offset, append) {
        if (!this._dbus)
            return;

        if (append)
            this._loadingMore = true;
        else
            this._loading = true;
        this._render();

        let requestSerial = ++this._requestSerial;
        this._dbus.notesJsonRemote(offset, PAGE_SIZE, this._query, Lang.bind(this, function(response, error) {
            if (requestSerial !== this._requestSerial)
                return;

            this._loading = false;
            this._loadingMore = false;
            if (error) {
                let errorMessage = String(error);
                if (errorMessage.indexOf('org.freedesktop.DBus.Error.UnknownObject') < 0
                    && errorMessage.indexOf('org.freedesktop.DBus.Error.ServiceUnknown') < 0) {
                    global.logError(error, 'Failed to fetch QtNote notes');
                }
                this._available = false;
                this._hasMore = false;
                if (!append)
                    this._items = [];
                this._render();
                return;
            }

            try {
                let parsed = JSON.parse(response);
                let notes = Array.isArray(parsed.notes) ? parsed.notes : [];
                this._available = true;
                this._hasMore = parsed.hasMore === true;
                this._items = append ? this._items.concat(notes) : notes;
            } catch (parseError) {
                global.logError(parseError, 'Failed to parse QtNote notes');
                this._available = false;
                this._hasMore = false;
                if (!append)
                    this._items = [];
            }
            this._render();
        }));
    }

    _render() {
        if (!this._list)
            return;

        this._list.destroy_all_children();
        this._noteRows = [];

        if (!this._available) {
            this._list.add_actor(this._message(_('QtNote is not running')));
            return;
        }

        if (!this._loading && this._items.length === 0) {
            this._list.add_actor(this._message(this._query ? _('No notes match the search') : _('No notes yet')));
            return;
        }

        for (let i = 0; i < this._items.length; ++i) {
            let row = this._noteRow(this._items[i], i);
            this._noteRows.push(row);
            this._list.add_actor(row);
        }

        if (this._loading || this._loadingMore)
            this._list.add_actor(this._message(_('Loading...')));

        if (this._hasMore && !this._loading && !this._loadingMore)
            this._list.add_actor(this._loadMoreRow());
    }

    _noteRow(note, index) {
        let row = new St.Button({
            can_focus: true,
            x_expand: true,
            x_align: Clutter.ActorAlign.FILL,
            style_class: 'qtnote-note-row',
        });
        let title = note.title && note.title.length > 0 ? note.title : _('Untitled Note');
        let titleLabel = new St.Label({
            text: title,
            x_expand: true,
            x_align: Clutter.ActorAlign.FILL,
            y_align: Clutter.ActorAlign.CENTER,
        });
        titleLabel.clutter_text.set_line_alignment(
            St.Widget.get_default_direction() === St.TextDirection.RTL
                ? Pango.Alignment.RIGHT
                : Pango.Alignment.LEFT);
        row.set_child(titleLabel);
        row.connect('clicked', Lang.bind(this, function() {
            this._activateNote(index);
        }));
        row.connect('key-press-event', Lang.bind(this, function(actor, event) {
            return this._onNoteKeyPress(index, event);
        }));
        return row;
    }

    _onSearchKeyPress(actor, event) {
        let symbol = event.get_key_symbol();
        if (symbol === Clutter.KEY_Down || symbol === Clutter.KEY_KP_Down) {
            if (this._noteRows.length > 0)
                this._noteRows[0].grab_key_focus();
            return true;
        }
        if (symbol === Clutter.KEY_Up || symbol === Clutter.KEY_KP_Up) {
            if (this._noteRows.length > 0)
                this._noteRows[this._noteRows.length - 1].grab_key_focus();
            return true;
        }
        if (symbol === Clutter.KEY_Return || symbol === Clutter.KEY_KP_Enter) {
            if (this._items.length > 0)
                this._activateNote(0);
            return true;
        }
        return false;
    }

    _onNoteKeyPress(index, event) {
        let symbol = event.get_key_symbol();
        if (symbol === Clutter.KEY_Down || symbol === Clutter.KEY_KP_Down) {
            if (index + 1 < this._noteRows.length)
                this._noteRows[index + 1].grab_key_focus();
            return true;
        }
        if (symbol === Clutter.KEY_Up || symbol === Clutter.KEY_KP_Up) {
            if (index > 0) {
                this._noteRows[index - 1].grab_key_focus();
            } else {
                this._searchEntry.clutter_text.grab_key_focus();
            }
            return true;
        }
        if (symbol === Clutter.KEY_Return || symbol === Clutter.KEY_KP_Enter) {
            this._activateNote(index);
            return true;
        }
        return false;
    }

    _activateNote(index) {
        let note = this._items[index];
        if (!note)
            return;

        this._call('openNote', note.storageId, note.id);
        this.menu.close();
    }

    _message(text) {
        return new St.Label({
            text: text,
            x_align: Clutter.ActorAlign.CENTER,
            style_class: 'qtnote-message',
        });
    }

    _loadMoreRow() {
        let row = new St.Button({
            can_focus: true,
            x_expand: true,
            style_class: 'qtnote-note-row qtnote-load-more-row',
        });
        row.set_child(new St.Label({
            text: _('Load more'),
            x_expand: true,
            x_align: Clutter.ActorAlign.CENTER,
            y_align: Clutter.ActorAlign.CENTER,
        }));
        row.connect('clicked', Lang.bind(this, this._loadMore));
        return row;
    }

    _call(method) {
        if (!this._dbus)
            return;

        let args = Array.prototype.slice.call(arguments, 1);
        let callback = function(result, error) {
            if (error)
                global.logError(error, 'Failed to call QtNote.' + method);
        };

        switch (method) {
        case 'openNote':
            this._dbus.openNoteRemote(args[0], args[1], callback);
            break;
        case 'createNote':
            this._dbus.createNoteRemote(callback);
            break;
        case 'showNoteManager':
            this._dbus.showNoteManagerRemote(callback);
            break;
        case 'showOptions':
            this._dbus.showOptionsRemote(callback);
            break;
        case 'showAbout':
            this._dbus.showAboutRemote(callback);
            break;
        case 'quit':
            this._dbus.quitRemote(callback);
            break;
        default:
            break;
        }
    }
}

function main(metadata, orientation, panelHeight, instanceId) {
    return new QtNoteApplet(metadata, orientation, panelHeight, instanceId);
}
