var ND_jsonI =
{
    jsWriteEvent: function (data, filename) {
        console.log("File output: \n", UTF8ToString(data));

        const a = document.createElement('a');
        const file = new Blob([UTF8ToString(data)], { type: 'text/plain' });

        a.href = URL.createObjectURL(file);
        a.download = UTF8ToString(filename);
        a.click();

        URL.revokeObjectURL(a.href);
    }
}

mergeInto(LibraryManager.library, ND_jsonI);