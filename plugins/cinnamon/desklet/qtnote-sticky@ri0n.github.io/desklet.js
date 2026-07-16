const Desklet = imports.ui.desklet;
const Gio = imports.gi.Gio;
const Lang = imports.lang;
const Pango = imports.gi.Pango;
const PopupMenu = imports.ui.popupMenu;
const St = imports.gi.St;

const BUS_NAME = 'com.github.ri0n.QtNote';
const OBJECT_PATH = '/QtNote';

const QTNOTE_IFACE_XML = `
<node>
  <interface name="com.github.ri0n.QtNote">
    <method name="stickyNoteForPresentationJson">
      <arg type="s" name="presentationId" direction="in"/>
      <arg type="s" name="response" direction="out"/>
    </method>
    <method name="openStickyNote">
      <arg type="s" name="stickyId" direction="in"/>
    </method>
    <method name="unpinStickyNote">
      <arg type="s" name="stickyId" direction="in"/>
    </method>
    <signal name="stickyNotesChanged"/>
  </interface>
</node>`;

const QtNoteProxy = Gio.DBusProxy.makeProxyWrapper(QTNOTE_IFACE_XML);

function _(text) {
    return text;
}

class QtNoteStickyDesklet extends Desklet.Desklet {
    constructor(metadata, deskletId) {
        super(metadata, deskletId);
        this._stickyId = '';
        this._signalId = 0;
        this._ownerSignalId = 0;

        this._box = new St.BoxLayout({
            vertical: true,
            reactive: true,
            style_class: 'qtnote-sticky',
        });
        this._title = new St.Label({style_class: 'qtnote-sticky-title'});
        this._title.clutter_text.ellipsize = Pango.EllipsizeMode.END;
        this._box.add_actor(this._title);

        this._body = new St.Label({style_class: 'qtnote-sticky-body'});
        this._body.clutter_text.line_wrap = true;
        this._body.clutter_text.line_wrap_mode = Pango.WrapMode.WORD_CHAR;
        this._body.clutter_text.ellipsize = Pango.EllipsizeMode.END;
        this._box.add_actor(this._body);
        this.setContent(this._box);

        let editItem = new PopupMenu.PopupIconMenuItem(_('Edit Note'), 'document-edit-symbolic', St.IconType.SYMBOLIC);
        editItem.connect('activate', Lang.bind(this, this._open));
        this._menu.addMenuItem(editItem);
        let unpinItem = new PopupMenu.PopupIconMenuItem(_('Unpin'), 'window-close-symbolic', St.IconType.SYMBOLIC);
        unpinItem.connect('activate', Lang.bind(this, this._unpin));
        this._menu.addMenuItem(unpinItem);

        this._proxy = new QtNoteProxy(Gio.DBus.session, BUS_NAME, OBJECT_PATH,
            Lang.bind(this, this._onProxyReady));
        this._showMessage(_('Connecting to QtNote…'));
    }

    _onProxyReady(proxy, error) {
        if (error) {
            global.logError(error, 'Failed to connect QtNote sticky desklet');
            this._showMessage(_('QtNote is not running'));
            return;
        }
        this._signalId = this._proxy.connectSignal('stickyNotesChanged', Lang.bind(this, this._refresh));
        this._ownerSignalId = this._proxy.connect('notify::g-name-owner', Lang.bind(this, function() {
            if (this._proxy.get_name_owner())
                this._refresh();
            else
                this._showMessage(_('QtNote is not running'));
        }));
        this._refresh();
    }

    _refresh() {
        this._proxy.stickyNoteForPresentationJsonRemote(String(this.instance_id),
            Lang.bind(this, function(result, error) {
                if (error) {
                    global.logError(error, 'Failed to load QtNote sticky note');
                    this._showMessage(_('Unable to load note'));
                    return;
                }
                try {
                    let payload = Array.isArray(result) ? result[0] : result;
                    let note = JSON.parse(payload || '{}');
                    if (!note.stickyId) {
                        this._showMessage(_('Note is unavailable'));
                        return;
                    }
                    this._stickyId = note.stickyId;
                    this._title.text = note.title || _('[No Title]');
                    this._body.text = note.body || '';
                } catch (parseError) {
                    global.logError(parseError, 'Failed to parse QtNote sticky note');
                    this._showMessage(_('Unable to load note'));
                }
            }));
    }

    _showMessage(message) {
        this._title.text = _('QtNote');
        this._body.text = message;
    }

    _open() {
        if (this._stickyId)
            this._proxy.openStickyNoteRemote(this._stickyId, function() {});
    }

    _unpin() {
        if (this._stickyId)
            this._proxy.unpinStickyNoteRemote(this._stickyId, function() {});
    }

    on_desklet_clicked(event) {
        if (event.get_click_count && event.get_click_count() >= 2)
            this._open();
    }

    on_desklet_removed(deleteConfig) {
        if (this._signalId)
            this._proxy.disconnectSignal(this._signalId);
        if (this._ownerSignalId)
            this._proxy.disconnect(this._ownerSignalId);
        if (deleteConfig && this._stickyId)
            this._proxy.unpinStickyNoteRemote(this._stickyId, function() {});
        this._signalId = 0;
        this._ownerSignalId = 0;
    }
}

function main(metadata, deskletId) {
    return new QtNoteStickyDesklet(metadata, deskletId);
}
