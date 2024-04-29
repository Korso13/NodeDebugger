# version 0.0.0.7
# надо установить opencv
# pip install opencv-python
# запускать из корня проекта
# 1. в resources/textures с вложенными папками ищет все файлы типа .png, .jpg определяет их разрешение
# 2. в существующий resources/config/resources.json в существующую секцию textures добавляет "[имя файла без расширения]":"[путь к файлу из папки resources + имя файла + расширение]" если такого пути еще нет
# 3. в существующий resources/config/meshesContainerSettings.json в существующую секцию settings добавляет настройки текстуры "name" : [имя текстуры из resources.json], и далее по схеме:
#    {
#      "name": [имя текстуры из resources.json],
#      "source": "",
#      "texture_name": "",
#      "renderOrder": 1,
#      "scale": 1.0,
#      "plane": {
#        "x": [ширина текстуры/100],
#        "y": [высота текстуры/100]
#      },
#      "pos": {"x": 0.0,"y": 0.0,"z": 0.0},
#      "rotate": {"x": 1.0,"y": 1.0,"z": 1.0}
#    }
# если данное имя есть - проверяет разрешение. если отличается от разрешения файла - исправляет на то, что в файле.
# удаление отсутствующих файлов (в resources и meshesContainerSettings(если CleanMeshContainer == True))(проверка по пути к файлу)
# Меши перечисленные в ExcludeMeshes не будут удаляться из meshesContainerSettings при отстутсвии ресурса и для них не будут создаваться записи в meshesContainerSettings даже при наличии ресурса (для переиспользуемых/универсальных записей в меш контейнере, например для кусков карт и кнопок звука) 
# перенос строк
# перенос строк нормальный
# Добавлена табуляция в переносы
# fix первой записи
# Fix детектора ресурсов лежащих в корне Textures

import cv2
import json
from os import listdir
from os.path import isfile, join
import os 

cur_path = os.path.dirname(os.path.realpath(__file__))
#print(cur_path)
dir_path = cur_path + "/resources/textures"
#print(dir_path)

ExcludeMeshes = ["sound_off", "installBtnGame_Google", "target"] #use it for textures you create with different MeshContainer record setting
CleanMeshContainer = True #Удаляет из MeshContainerSettings.json записи для которых нет соответсвующей текстуры в resources (за исключением записей в ExcludeMeshes)

def checkIfExcluded(meshRec):
    for exclusionName in ExcludeMeshes:
        if(meshRec == exclusionName):
            return True
    
    return False

class MeshFile:
    def __init__(self, folder, fileName, ext, w, h):
        self.folder = folder
        self.fileName = fileName
        self.resourceName = fileName
        self.ext = ext
        self.w = w
        self.h = h      

meshes = []

def getResolution(folder, fileName):
    # loading the image
    #print("getresolution " + fileName)
    img = cv2.imread( f'{folder}/{fileName}')

    # fetching the dimensions
    wid = img.shape[1]
    hgt = img.shape[0]
    fn, ext = os.path.splitext(fileName)
    # displaying the dimensions
    folder = folder[len(dir_path) + 1 : len(folder)]
    #print(folder +" " + fn + " " + ext + " " + str(wid) + "x" + str(hgt))
    meshes.append(MeshFile(folder, fn, ext, wid/100, hgt/100))
    

def getFilesInFolder(mypath):
    onlyfiles = [f for f in listdir(mypath) if isfile(join(mypath, f))]
    print(str(onlyfiles))

def getResolutionsInFolder(dirFolder):
    print("getResolutionsInFolder ", dirFolder  )
    #files = getFilesInFolder(dirFolder)
    for f in listdir(dirFolder):
        print("f =" , f)
        
        if os.path.isdir(dirFolder +"/"+f):
            print("isfolder:" + f)
            getResolutionsInFolder(dirFolder +"/"+f)
        else:
            filename, file_extension = os.path.splitext(f)
            print("extension = [" + file_extension + "]")
            if file_extension == ".jpg" or file_extension == ".png":
                getResolution(dirFolder, f)
            
        print("^^^^^^^")


getResolutionsInFolder(dir_path)

def printMeshes(meshes):
    for m in meshes:
        print(m.folder +" " + m.fileName + " " + m.ext + " " + str(m.w) + " " + str(m.h))
        
printMeshes(meshes)

def createResources(meshes):
    
    
    s = ""
    for m in meshes:
        s = s + "\n\"" + m.fileName + "\": " + m.folder+"/"+m.fileName+m.ext
    
    with open("resource.txt", "w") as file:
        print("file created")
        file.write(s)
        file.close()


#createResources(meshes)


def createMeshContainer(meshes):
      
    s = ""
    for m in meshes:
        #s = s + "\n\"" + m.fileName + "\": " + m.folder+"/"+m.fileName+m.ext
        s = s+ "{\n" + "    \"name\": \"" + m.fileName + "\",\n"
        s = s + "    \"source\": \"\",\n"
        s = s + "    \"texture_name\": \"\",\n"
        s = s + "    \"renderOrder\": 1,\n"
        s = s + "    \"scale\": 1.0,\n"
        s = s + "    \"plane\": {\"x\": " + m.w + ", \"y\": " + m.h +" },\n"
        s = s + "    \"pos\": {\"x\": 0.0, \"y\":0.0, \"z\":0.0 },\n"
        s = s + "    \"rotate\": {\"x\": 0.0, \"y\":0.0, \"z\":0.0 }\n"
        s = s +"},\n"
    
    s = s.removesuffix(",")
    with open("meshescontainer.txt", "w") as file:
        print("file created")
        file.write(s)
        file.close()

#createMeshContainer(meshes)

def checkFileInResources(file_path, texturesArray):
    print("Checking if file " + file_path + " is in textures")
    for fileResName in texturesArray:
        # print("Checking against record " + fileResName + " with path " + texturesArray[fileResName])
        if(file_path == texturesArray[fileResName]):
            return fileResName
    return ""



def checkResources():
    resurceFilePath = cur_path +"/resources/config/resources.json"
    print("============checkResources================")
    print("resPath = ", resurceFilePath)
    meshesDuplicateList = []
    with open(resurceFilePath, "r") as res_file:
        data = json.load(res_file)
        for m in meshes:
            print("=====checkfile " + m.fileName + "=======")
            #print("dump " + str(data["textures"]))
            checkTexturePath = "textures/"
            if len (m.folder) > 0:
                checkTexturePath = checkTexturePath + m.folder+"/"
            checkResult = checkFileInResources(checkTexturePath + m.fileName + m.ext, data["textures"])
            if checkResult == "":
                print(m.fileName + " not found")
                if m.folder:
                    data["textures"][m.fileName] =  "textures/"+m.folder+"/"+m.fileName+m.ext
                else:
                    data["textures"][m.fileName] =  "textures/"+m.fileName+m.ext
            else:
                m.resourceName = checkResult
                # meshesDuplicateList.append(m)
        
        # for duplicate in meshesDuplicateList:
        #     meshes.remove(duplicate)
        # print("Removed " + str(meshesDuplicateList.count) + " records from meshes due to duplication")
        printMeshes(meshes)   

        remlist = []
        for resName in data["textures"]:
            resourcePathRec = data["textures"][resName]
            result = False
            print("Checking if resource " + resName + " with path " + resourcePathRec + " still exists")

            for m in meshes:
                # print("mbef = "+"textures/"+m.folder+"/"+m.fileName+m.ext)
                foundTexturePath = "textures/"
                if len (m.folder) > 0:
                    foundTexturePath = foundTexturePath + m.folder+"/"
                foundTexturePath = foundTexturePath + m.fileName+m.ext
                # print("mbef = "+foundTexturePath)
                
                if foundTexturePath == resourcePathRec:
                    m.resourceName = resName
                    result = True
                    break
                # print("txt "+txt) 
                    
            if not result:
                print("!! Failed to find " + resName + " path " + data["textures"][resName] + " in detected files. Removing !!" )
                remlist.append(resName)
                
            else:
                print("Found " + resName + " with path " + data["textures"][resName] + " in detected files" )
                

        for resNameRecToDel in remlist:
            data["textures"].pop(resNameRecToDel)
            print("Deleted " + resNameRecToDel )

        with open(resurceFilePath, 'w') as json_file:
            json.dump(data, json_file,  indent=4)
checkResources()

def checkJsonExists(name, dataSettings):
    print("checkJsonExists ", name)
    i = -1
    for meshSett in dataSettings:
        #print("checkJsonExists " +str( meshSett))
        #print("checkJsonExists name " +str( meshSett["name"]))
        i = i + 1
        if meshSett["name"] == name:
            return i
    return -1

def deleteRemovedMeshes(jsonData):
    recordsToDelete = []
    for meshContainerRecord in jsonData["settings"]:
        recordFound = False
        for m in meshes:
            # print("Checking mesh " + meshContainerRecord["name"] + " against texture record " + m.resourceName)
            if(m.resourceName == meshContainerRecord["name"]):
                recordFound = True
                break
        if(not recordFound and not checkIfExcluded(meshContainerRecord["name"])):
            print("Removing record " + meshContainerRecord["name"] + " which has no resource")
            recordsToDelete.append(meshContainerRecord)
    
    for deleteRec in recordsToDelete:
        jsonData["settings"].remove(deleteRec)
    
    return jsonData

def checkMeshContainer():
    meshFilePath = cur_path +"/resources/config/meshesContainerSettings.json"
    print("============checkMeshContainer================")
    print("resPath = ", meshFilePath)
    with open(meshFilePath, "r") as meshes_file:
        data = json.load(meshes_file)
        for m in meshes:
            # print("textures found")
            print("check record " + m.resourceName)
            #print("dump " + str(data["textures"]))
            i = checkJsonExists( m.resourceName, data["settings"])
            if i > -1:
                #print(" checkMeshContainer"  + str(data["settings"][i]))
                if data["settings"][i]["plane"]["x"] != m.w or data["settings"][i]["plane"]["y"] != m.h: 
                    data["settings"][i]["plane"]["x"] =  m.w
                    data["settings"][i]["plane"]["y"] =  m.h
                    
            else: 
                if(not m.resourceName.startswith("ES_") and not checkIfExcluded(m.resourceName)):
                    #sJson = "{\"name\":\"" + m.fileName + "\"," + "\"source\":\"\", \"renderOrder\":1, \"plane\":{\"x\":" + m.w + ",\"y\":" + m.h + "},"
                    #sJson = sJson + "\"pos\":{\"x\":0.0, \"y\":0.0, \"z\":0.0}," + "\"scale\":{\"x\":1.0, \"y\":1.0, \"z\":1.0}" + "}"
                    #print(sJson)
                    data["settings"].append({"name":m.resourceName,"source":"", "texture_name": "", "renderOrder":1, "scale":1.0, "plane":{"x":m.w, "y":m.h}, "pos":{"x":0.0,"y":0.0,"z":0.0}, "rotate":{"x":0.0,"y":0.0,"z":0.0}})
        
        if(CleanMeshContainer):
            data = deleteRemovedMeshes(data)
        s = str(data)
        s = s.replace("'", "\"")
        s = s.replace("}},", "}\n\t\t},")
        s = s.replace("{\"name\"", "\n\t\t{\n\t\t\t\"name\"")
        s = s.replace(", \"texture_name\"", ",\n\t\t\t\"texture_name\"")
        s = s.replace(", \"source\"", ",\n\t\t\t\"source\"")
        s = s.replace(", \"renderOrder\"", ",\n\t\t\t\"renderOrder\"")
        s = s.replace(", \"plane\"", ",\n\t\t\t\"plane\"")
        s = s.replace(", \"pos\"", ",\n\t\t\t\"pos\"")
        s = s.replace(", \"rotate\"", ",\n\t\t\t\"rotate\"")
        s = s.replace(", \"scale\"", ",\n\t\t\t\"scale\"")
        s = s.replace("{\"settings\": [", "{\n\t\"settings\":[")
        s = s.replace("}}]}", "}\n\t\t}\n\t]\n}")


        
       # s = s.replace(", \"", ",\n\"")

        with open(meshFilePath, 'w') as json_file:
            #json.dump(data, json_file, indent=4)
            #str = json.dump(data)
            json_file.write(s)




checkMeshContainer()

print("ready")














        
    
