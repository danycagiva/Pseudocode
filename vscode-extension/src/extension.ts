import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';

let ausgabeKanal: vscode.OutputChannel;
let fehlersammlung: vscode.DiagnosticCollection;

// Fehlerformat: datei.pseudo:3:5: Fehler: Nachricht
const FEHLER_MUSTER = /^(.+):(\d+):(\d+):\s+(?:Fehler|Laufzeitfehler):\s+(.+)$/;

export function activate(context: vscode.ExtensionContext) {
    ausgabeKanal = vscode.window.createOutputChannel('PseudoCode');
    fehlersammlung = vscode.languages.createDiagnosticCollection('pseudocode');

    const befehl = vscode.commands.registerCommand('pseudocode.ausfuehren', ausfuehren);

    context.subscriptions.push(ausgabeKanal);
    context.subscriptions.push(fehlersammlung);
    context.subscriptions.push(befehl);

    // Fehler beim Speichern zuruecksetzen
    context.subscriptions.push(
        vscode.workspace.onDidSaveTextDocument(doc => {
            if (doc.languageId === 'pseudocode') {
                fehlersammlung.delete(doc.uri);
            }
        })
    );
}

async function ausfuehren() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showWarningMessage('Keine PseudoCode-Datei geoeffnet.');
        return;
    }

    const datei = editor.document;
    if (datei.languageId !== 'pseudocode') {
        vscode.window.showWarningMessage('Aktive Datei ist keine PseudoCode-Datei (.pseudo).');
        return;
    }

    // Datei speichern falls noetig
    if (datei.isDirty) {
        await datei.save();
    }

    const config = vscode.workspace.getConfiguration('pseudocode');
    const interpreter = config.get<string>('interpreterPfad', 'pseudocode');
    const dateiPfad = datei.fileName;

    fehlersammlung.delete(datei.uri);
    ausgabeKanal.clear();
    ausgabeKanal.show(true);
    ausgabeKanal.appendLine(`▶  ${path.basename(dateiPfad)}\n`);

    const startzeit = Date.now();

    cp.execFile(interpreter, [dateiPfad], (err, stdout, stderr) => {
        const dauer = ((Date.now() - startzeit) / 1000).toFixed(2);

        if (stdout) ausgabeKanal.append(stdout);

        if (stderr) {
            const diagnostics: vscode.Diagnostic[] = [];

            for (const zeile of stderr.split('\n')) {
                const treffer = FEHLER_MUSTER.exec(zeile.trim());
                if (treffer) {
                    const [, , zeilenNr, spaltenNr, nachricht] = treffer;
                    const linie = Math.max(0, parseInt(zeilenNr, 10) - 1);
                    const spalte = Math.max(0, parseInt(spaltenNr, 10) - 1);

                    const bereich = new vscode.Range(linie, spalte, linie, 999);
                    const diag = new vscode.Diagnostic(
                        bereich,
                        nachricht,
                        vscode.DiagnosticSeverity.Error
                    );
                    diagnostics.push(diag);
                }
                if (zeile.trim()) ausgabeKanal.appendLine(zeile);
            }

            if (diagnostics.length > 0) {
                fehlersammlung.set(datei.uri, diagnostics);
            }
        }

        if (err) {
            ausgabeKanal.appendLine(`\n✗ Abgebrochen nach ${dauer}s`);
        } else {
            ausgabeKanal.appendLine(`\n✓ Fertig in ${dauer}s`);
        }
    });
}

export function deactivate() {
    fehlersammlung?.dispose();
    ausgabeKanal?.dispose();
}
