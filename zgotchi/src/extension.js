// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
const vscode = require('vscode');

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
//var count = 0;
//var counter = 0;

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
	const disposable = vscode.workspace.onDidChangeTextDocument(event => {
        console.log('Document changed:', event.contentChanges);
		vscode.window.showInformationMessage('Document changed:' + event.contentChanges);
    });

	context.subscriptions.push(disposable);
}

// This method is called when your extension is deactivated
function deactivate() {}

module.exports = {
	activate,
	deactivate
}
