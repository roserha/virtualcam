using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using TMPro;
using System.IO;
using UnityEngine.UI;

public class Buttons : MonoBehaviour
{
    string FilePath;
    string FolderLocation;
    public StreamdeckEmotions StreamdeckJSON;
    public TextMeshProUGUI fileLocation;
    public TextMeshProUGUI DebugButtonText;
    public TextMeshProUGUI EyeShiftText;
    public TextMeshProUGUI VolumeText;

    // Start is called before the first frame update
    void Start()
    {
        StreamdeckJSON = new StreamdeckEmotions(0, 0, 10, 0);
        FolderLocation = GetAndroidExternalStoragePath() + "/Stremdeck/";
        if(!Directory.Exists(FolderLocation))
        {
            Directory.CreateDirectory(FolderLocation);
        }
        FilePath = FolderLocation + "emotion.json";
        File.WriteAllText(FilePath, JsonUtility.ToJson(StreamdeckJSON));
    }

    public string GetAndroidExternalStoragePath()
    {
        string path = "";
        try
        {
            AndroidJavaClass jc = new AndroidJavaClass("android.os.Environment");
            path = jc.CallStatic<AndroidJavaObject>("getExternalStorageDirectory").Call<string>("getAbsolutePath");
            return path;
        }
        catch (Exception e)
        {
            Debug.Log(e.Message);
            return Application.dataPath + "\\..\\";
        }
    }

    public void ChangeEmotion(int emotionCode)
    {
        StreamdeckJSON.e = emotionCode;
        StreamdeckJSON.t = UnityEngine.Random.Range(10000,999999);
        File.WriteAllText(FilePath, JsonUtility.ToJson(StreamdeckJSON));
    }

    public void ToggleEyeshift()
    {
        StreamdeckJSON.s ^= 1;
        File.WriteAllText(FilePath, JsonUtility.ToJson(StreamdeckJSON));
        EyeShiftText.text = "Eye Shift Enabled: " + (StreamdeckJSON.s == 1);
    }

    public void ToggleDebug()
    {
        StreamdeckJSON.d ^= 1;
        File.WriteAllText(FilePath, JsonUtility.ToJson(StreamdeckJSON));
        DebugButtonText.text = "Debugging: " + (StreamdeckJSON.d == 1);
        if(StreamdeckJSON.d == 1) fileLocation.text = FilePath;
        else fileLocation.text = "Stremdeck Swift - by Ricky Horizon";
    }

    public void VolumeAdd(int volAdd)
    {
        StreamdeckJSON.v += volAdd;
        File.WriteAllText(FilePath, JsonUtility.ToJson(StreamdeckJSON));
        VolumeText.text = "Mic Volume: \n" + StreamdeckJSON.v;
    }
}

// Variable names must be as short as possible, to ensure 
// fast copies from Android Debug Bridge. Next to each va
// riable, there is a comment with the complete variable.
[Serializable]
public class StreamdeckEmotions
{
    public int e/*motions*/;
    public int /*eye*/s/*hift*/;
    public int v/*olume*/;
    public int d/*ebug*/;
    public int t/*ouchId*/; //This is here to ensure that when you press on the same emotion, its cooldown will be reset

    public StreamdeckEmotions(int emotion, int eyeshift, int volume, int debug)
    {
        e = emotion;
        s = eyeshift;
        v = volume;
        d = debug;
        t = UnityEngine.Random.Range(10000,999999);
    }
}