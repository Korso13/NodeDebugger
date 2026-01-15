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
    },

    jsReadImage: function (self, callback) {
        var input = document.createElement('input');
        input.type = 'file';
        var result = input.click();

        input.onchange = e => {
            var file = input.files[0];
            console.warn(file.name);

            var reader = new FileReader();
            reader.readAsDataURL(file);

            // here we tell the reader what to do when it's done reading...
            reader.onload = readerEvent => {
                var content = readerEvent.target.result; // this is the content!

                console.log("File " + file.name + " read. Calling callback!");
                const ptrStr = allocateUTF8(content);
                wasmTable.get(callback)(self, ptrStr); // ptr is char*
                Module._free(ptrStr);
            };

        }

    },

    jsReadJson: function (self, callback) {
        var input = document.createElement('input');
        input.type = 'file';
        var result = input.click();

        input.onchange = e => {
            var file = input.files[0];
            console.warn(file.name);

            var reader = new FileReader();
            reader.readAsText(file);

            // here we tell the reader what to do when it's done reading...
            reader.onload = readerEvent => {
                var content = readerEvent.target.result; // this is the content!

                console.log("File " + file.name + " read. Calling callback!");
                const ptrJsonStr = allocateUTF8(content);
                wasmTable.get(callback)(self, ptrJsonStr); // ptrStr is char*
                Module._free(ptrJsonStr);

            };

        }

    },

    jsReadJsonEx: function (self, callback) {
        var input = document.createElement('input');
        input.type = 'file';
        var result = input.click();

        input.onchange = e => {
            var file = input.files[0];
            console.warn(file.name);

            var reader = new FileReader();
            reader.readAsText(file);

            // here we tell the reader what to do when it's done reading...
            reader.onload = readerEvent => {
                var content = readerEvent.target.result; // this is the content!

                console.log("File " + file.name + " read. Calling callback!");
                const ptrJsonStr = allocateUTF8(content);
                const ptrFileNameStr = allocateUTF8(file.name);
                const ptrFilePathStr = allocateUTF8(file.webkitRelativePath); //not returning what needed :(
                wasmTable.get(callback)(self, ptrJsonStr, ptrFileNameStr, ptrFilePathStr); // ptrJsonStr, ptrFileNameStr, ptrFilePathStr are char*
                Module._free(ptrJsonStr);
                Module._free(ptrFileNameStr);
                Module._free(ptrFilePathStr);
            };

        }

    }


}

mergeInto(LibraryManager.library, ND_jsonI);