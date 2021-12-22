/*
QtNote - Simple note-taking application
Copyright (C) 2010 Sergei Ilinykh

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contacts:
E-Mail: rion4ik@gmail.com XMPP: rion@jabber.ru
*/

#ifndef NOTEMANAGERDLG_H
#define NOTEMANAGERDLG_H

#include <QDialog>
#include <QModelIndex>

namespace Ui {
class NoteManagerDlg;
}

class QItemSelection;

namespace QtNote {

class NotesModel;
class NotesSearchModel;
class Main;

class NoteManagerDlg : public QDialog {
    Q_OBJECT

public:
    explicit NoteManagerDlg(Main *qtnote);
    ~NoteManagerDlg();

signals:
    void showNoteRequested(const QString &, const QString &);

protected:
    void changeEvent(QEvent *e);

private slots:
    void itemDoubleClicked(const QModelIndex &index);
    void currentRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void updateStats();

private:
    Ui::NoteManagerDlg *ui;
    NotesModel         *model;
    NotesSearchModel   *searchModel;
    Main               *qtnote;
};

}

#endif // NOTEMANAGERDLG_H
