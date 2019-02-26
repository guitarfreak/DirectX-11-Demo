import math
import maya.cmds as cmds

def maya_useNewAPI(): 
	pass

path = "C:/"

#exportMeshes(path + "custom.mesh")
#exportAnimation(path + "custom.anim")

print "Finished."



def selectAllJoints():
    cmds.select(allDagObjects=True, hierarchy=True) 
    selection = cmds.ls(selection=True, showType=True)
    
    for i in range(len(selection)):
        if selection[i] == "joint":
            selection = selection[i-1]
            break
    
    # Avoid soft lock.
    for i in range(100):
        parent = cmds.listRelatives(selection, parent=True)
        if parent is not None: selection = parent
        else: break
    
    cmds.select(selection)    
    cmds.select(hierarchy=True)

def selectAllMeshes():
    cmds.select(allDagObjects=True)
    selection = cmds.ls(selection=True)
    print selection
    
    filteredSelection = []
    for it in selection:
        relatives = cmds.listRelatives(it, children=True)
        if cmds.objectType(relatives[0]) == "mesh":
            filteredSelection.append(it)

    cmds.select(filteredSelection)

def exportMeshes(path):
    f = open(path, "w")
    
    oldSelection = cmds.ls(selection=True)
    
    selectAllJoints()
    
    # Bone values
    
    boneValues = []
    selection = cmds.ls(selection=True)
    
    offset = cmds.ls(selection, long=True)[0].count('|')
    
    for it in selection: 
        depth = cmds.ls(it, long=True)[0].count('|') - offset
        boneValues.append([depth, it])
    
    f.write("Bones: " + str(len(boneValues)) + "\n")
    for it in boneValues:
        f.write("%d %s\n" % (it[0], it[1]))
    f.write("\n")
    
    # Orientation values
    
    '''
    orientation = [cmds.joint(it, query=True, orientation=True, relative=True) for it in selection]
    f.write("Orientation: " + "\n")
    for it in orientation:
       f.write("%.6f %.6f %.6f\n" % (math.radians(it[0]), math.radians(it[1]), math.radians(it[2])))
    f.write("\n")
    '''
    
    # Frame data
    
    f.write("Pose: \n")  

    for bone in boneValues:
        b = bone[1]
        f.write("%.6f %.6f %.6f\n" % (cmds.getAttr(b+".translateX"), cmds.getAttr(b+".translateY"), cmds.getAttr(b+".translateZ")))
        f.write("%.6f %.6f %.6f\n" % (math.radians(cmds.getAttr(b+".rotateX")), math.radians(cmds.getAttr(b+".rotateY")), math.radians(cmds.getAttr(b+".rotateZ"))))
        f.write("%.6f %.6f %.6f\n" % (cmds.getAttr(b+".scaleX"), cmds.getAttr(b+".scaleY"), cmds.getAttr(b+".scaleZ")))     
        
        f.write("\n")
    
    
    
    # Meshes

    selectAllMeshes()

    objList = cmds.ls(sl=1, l=1)
    
    f.write("Count: %d\n" % len(objList))

    for shapeNode in objList:
        # Verts
        numVerts = maya.cmds.polyEvaluate(shapeNode, vertex=True)
        vertexValues = [maya.cmds.pointPosition("%s.vtx[%d]" % (shapeNode, p)) for p in range(numVerts)]

        # Skin
        skinClust = mel.eval('findRelatedSkinCluster ' + shapeNode)
        skinNameValues = [maya.cmds.skinPercent(skinClust, "%s.vtx[%d]" % (shapeNode, p), ignoreBelow=0.00001, query=True, transform=None) for p in range(numVerts)]
        skinWeightValues = [maya.cmds.skinPercent(skinClust, "%s.vtx[%d]" % (shapeNode, p), ignoreBelow=0.00001, query=True, value=True) for p in range(numVerts)]    

        # UVs
        texVertCount = maya.cmds.polyEvaluate(shapeNode, uvcoord=True)
        textureValues = [maya.cmds.getAttr("%s.uvpt[%d]" % (shapeNode,i)) for i in range(texVertCount)]
        
        # Normals
        numFaceNormals = 0
        vertexNormalValues = []
        faceNormals = []
        
        faceCount = maya.cmds.polyEvaluate(shapeNode, face=True)
        for face in range(faceCount):
            comp="%s.f[%d]" % (shapeNode,face)
            vertexFaces = maya.cmds.polyListComponentConversion(comp, fromFace=True, toVertexFace=True)
            vertexFaces = maya.cmds.filterExpand(comp, vertexFaces, selectionMask=70, expand=True)
            faceNormals.append([])
            for vertexFace in vertexFaces:
                vertexNormalValues.append(maya.cmds.polyNormalPerVertex(vertexFace, query=True, xyz=True))
                numFaceNormals += 1
                faceNormals[-1].append(numFaceNormals)
        
        # Faces
        numFaces = maya.cmds.polyEvaluate(shapeNode, face=True)
        faceValues = []
        vnIter = 0
        
        vertList = []
        vertNormalList = []
        vertTextureList = []
        for faceNum in range(numFaces):
            oFace = "%s.f[%d]" % (shapeNode, faceNum)    
        
            # Verts (v)
            faceVerts = maya.cmds.polyInfo(oFace, faceToVertex=True)    
            faceVerts = [int(fv)+1 for fv in faceVerts[0].split(":")[-1].strip().replace("  "," ").replace("  "," ").replace("  "," ").replace(" ",",").split(",")] 
            vertList.append(faceVerts)

            # Normals (vn)
            #vertNormalList.append(faceNormals[faceNum])
            
            if texVertCount > 0:
                # Texture (vt)
                tex = maya.cmds.polyListComponentConversion(oFace, fromFace=True, toUV=True)
                if len(tex) > 0:
                    tex = maya.cmds.filterExpand(tex, selectionMask=35, expand=True)         
                    tex = [int(i.split("map")[-1].strip("[]")) +1 for i in tex]
                
                # uv need to get in same order as vertex ordering
                tmpDict = {}
            
                for t in tex:
                    component="\n%s.map[%d]" % (shapeNode,t-1)
            
                    vertFromTex = maya.cmds.polyListComponentConversion(component, fromUV=True, toVertex=True)
                    if vertFromTex:
                        index=int(vertFromTex[0].split("[")[-1].split("]")[0]) + 1
                        tmpDict[index] = t
            
                orderedTex = []
                for vert in vertList[-1]:
                     orderedTex.append(tmpDict[vert])
                     vertTextureList.append(orderedTex)
            
                #
            
                face = [[faceVerts[i], orderedTex[i], faceNormals[faceNum][i]] for i in range(len(faceVerts))]
                faceValues.append(face)
                
            else:
                face = [[faceVerts[i], 1, faceNormals[faceNum][i]] for i in range(len(faceVerts))]
                faceValues.append(face)
            
        #

        f.write("Verts: " + str(len(vertexValues)) + "\n")
        for it in vertexValues: f.write("%.6f " % it[0] + "%.6f " % it[1] + "%.6f" % it[2] + "\n")
        f.write("\n")
        
        #f.write("UVs: " + str(len(textureValues)) + "\n")
        #for it in textureValues: f.write("%.6f " % it[0][0] + "%.6f" % it[0][1] + "\n")
        #f.write("\n")
        
        f.write("Normals: " + str(len(vertexNormalValues)) + "\n")
        for it in vertexNormalValues: f.write("%.6f " % it[0] + "%.6f " % it[1] + "%.6f" % it[2] + "\n")
        f.write("\n")

        f.write("Skin: " + str(len(vertexValues)) + "\n")
        for i in range(len(skinNameValues)):
            for name in skinNameValues[i]: f.write(name + " ")
            f.write("\n")
            for name in skinWeightValues[i]: f.write(str(name) + " ")
            f.write("\n")
        f.write("\n")
        
        f.write("Node: " + shapeNode + "\n")
        f.write("Faces: " + str(len(faceValues)) + "\n")

        if texVertCount > 0:
            for face in faceValues: 
                for v in face: f.write("%d/%d/%d " % (v[0], v[1], v[2]))
                f.write("\n")
            f.write("\n")
            
        else: 
            for face in faceValues: 
                for v in face: f.write("%d//%d " % (v[0], v[2]))
                f.write("\n")
            f.write("\n")
                    
    f.close()
    
    cmds.select(oldSelection)

def exportAnimation(path):
    f = open(path, "w")
    
    startTime = cmds.playbackOptions(query=True, animationStartTime=True)
    endTime = cmds.playbackOptions(query=True, animationEndTime=True)
    playbackSpeed = cmds.playbackOptions(query=True, playbackSpeed=True)
    fps = cmds.playbackOptions(query=True, framesPerSecond=True)
    loop = cmds.playbackOptions(query=True, loop=True)
    
    f.write("StartTime: %d \n" % startTime)
    f.write("EndTime: %d \n" % endTime)
    f.write("Speed: %f \n" % playbackSpeed)
    f.write("Fps: %d \n" % fps)
    f.write("Loop: %s \n" % loop)
    
    f.write("\n")
    
    # Bone values

    oldSelection = cmds.ls(selection=True)
   
    selectAllJoints() 
    
    selection = cmds.ls(selection=True)
    offset = cmds.ls(selection, long=True)[0].count('|')
        
    boneValues = []

    for it in selection: 
        depth = cmds.ls(it, long=True)[0].count('|') - offset
        boneValues.append([depth, it])
    
    f.write("Bones: " + str(len(boneValues)) + "\n")
    for it in boneValues:
        f.write("%d %s\n" % (it[0], it[1]))
    f.write("\n")   
    
    # Frame data
    
    f.write("Frames: %d" % (endTime - startTime + 1) + "\n")
    
    for time in range(int(startTime), int(endTime) + 1):
        f.write("Frame: %d" % time + "\n")
        cmds.currentTime(time)    
    
        for bone in boneValues:
            b = bone[1]
            f.write("%.6f %.6f %.6f\n" % (cmds.getAttr(b+".translateX"), cmds.getAttr(b+".translateY"), cmds.getAttr(b+".translateZ")))
            f.write("%.6f %.6f %.6f\n" % (math.radians(cmds.getAttr(b+".rotateX")), math.radians(cmds.getAttr(b+".rotateY")), math.radians(cmds.getAttr(b+".rotateZ"))))
            f.write("%.6f %.6f %.6f\n" % (cmds.getAttr(b+".scaleX"), cmds.getAttr(b+".scaleY"), cmds.getAttr(b+".scaleZ")))    
            
            f.write("\n")
    f.close()
    
    cmds.select(oldSelection)
