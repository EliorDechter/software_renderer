/* ========================================================================
   $File: C:\work\handmade\dota\dotahero.cpp $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright by Molly Rocket, Inc., All Rights Reserved. $
   ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define internal static

struct string_list
{
    int Count;
    char **Strings;
};

string_list ExistingHeroes;
string_list PrimaryNoun; // NOTE(casey): Beast, War, etc.
string_list SecondaryVerb; // NOTE(casey): Rider, etc.
string_list SecondaryNoun; // NOTE(casey): King, etc.
string_list SecondarySuffix; // NOTE(casey): back, werk, etc.
string_list ConsonantVowel; // NOTE(casey): ki, ra, etc.
string_list ConsonantOnly; // NOTE(casey): ck, etc.
string_list VowelOnly; // NOTE(casey): a, e, etc.

#if 0
Anti-Mage
Vengeful Spirit
Lone Druid
Ancient Apparition
Naga Siren
Nyx Assassin
Keeper of the Light
Nature's Prophet
Outworld Devourer
Queen of Pain
Skywrath Mage
Dark Seer
Dark Willow
Terrorblade


Alchemist
Axe
Centaur
Doom
Io
Lycan
Magnus
Mars
Omniknight
Phoenix
Pudge
Sven
Tiny
Tusk
Underlord
Undying
Juggernaut
Luna
Medusa
Morphling
Razor
Sniper
Spectre
Ursa
Viper
Weaver
Dazzle
Disruptor
Enchantress
Enigma
Grimstroke
Invoker
Lion
Oracle
Puck
Rubick
Silencer
Techies
Tinker
Visage
Warlock
Zeus
#endif

internal string_list
LoadStringList(char *FileName)
{
    string_list Result = {};
    
    FILE *File = fopen(FileName, "r");
    if(File)
    {
        char Buffer[4096];
        int ExpectedCount = 0;
        while(!feof(File))
        {
            fgets(Buffer, sizeof(Buffer), File);
            ++ExpectedCount;
        }
        Result.Strings = (char **)malloc(ExpectedCount*sizeof(char *));
        fseek(File, 0, SEEK_SET);
        while(!feof(File))
        {
            fgets(Buffer, sizeof(Buffer), File);
            size_t Length = strlen(Buffer);
            for(int Clean = 0;
                Clean < Length;
                ++Clean)
            {
                if((Buffer[Clean] == '\n') ||
                   (Buffer[Clean] == '\r'))
                {
                    Buffer[Clean] = 0;
                }
            }
            Length = strlen(Buffer);
            if(Length)
            {
                char **Dest = &Result.Strings[Result.Count++];
                *Dest = (char *)malloc(Length + 1);
                memcpy(*Dest, Buffer, Length + 1);
            }
        }
        fclose(File);
    }
    else
    {
        fprintf(stderr, "ERROR: Cannot open required file %s.\n", FileName);
    }
    
    if(Result.Count == 0)
    {
        char *ErrorStringSet[] =
        {
            "",
        };
        Result.Count = 1;
        Result.Strings = ErrorStringSet;
    }
    
    return(Result);
}

internal int
GetRandomCount(int Count)
{
    int Result = (rand()%Count);
    return(Result);
}

internal char *
Concat(char *A, char *B)
{
    char *Result;
    if(A && B)
    {
        size_t ALen = strlen(A);
        size_t BLen = strlen(B);
        Result = (char *)malloc(ALen + BLen + 1);
        memcpy(Result, A, ALen);
        memcpy(Result + ALen, B, BLen);
        Result[ALen + BLen] = 0;
    }
    else if(A)
    {
        Result = A;
    }
    else
    {
        Result = B;
    }
    return(Result);
}

internal char *
Concat(char *A, char *B, char *C)
{
    char *Result = Concat(A, Concat(B, C));
    return(Result);
}

internal char *
Concat(char *A, char *B, char *C, char *D)
{
    char *Result = Concat(A, B, Concat(C, D));
    return(Result);
}

internal char *
Concat(char *A, char *B, char *C, char *D, char *E)
{
    char *Result = Concat(A, B, C, Concat(D, E));
    return(Result);
}

internal char *
Concat(char *A, char *B, char *C, char *D, char *E, char *F)
{
    char *Result = Concat(A, B, C, D, Concat(E, F));
    return(Result);
}

internal char *
Clone(char *A)
{
    char *Result = 0;
    if(A)
    {
        size_t Len = strlen(A);
        Result = (char *)malloc(Len + 1);
        memcpy(Result, A, Len);
        Result[Len] = 0;
    }
    return(Result);
}

internal char *
CapitalizeFirst(char *A)
{
    char *Result = Clone(A);
    if(Result && *Result)
    {
        Result[0] = (char)toupper(Result[0]);
    }
    return(Result);
}

internal char *
RandomFrom(string_list List)
{
    char *Result = List.Strings[GetRandomCount(List.Count)];
    return(Result);
}

internal char *
GetSecondaryEnding(void)
{
    char *Result = "ERROR";
    switch(GetRandomCount(2))
    {
        case 0:
        {
            Result = RandomFrom(SecondaryVerb);
        } break;
        
        case 1:
        {
            Result = RandomFrom(SecondarySuffix);
        } break;
    }
    
    return(Result);
}

internal char *
GetFinalWord(void)
{
    char *Result = "ERROR";
    switch(GetRandomCount(2))
    {
        case 0:
        {
            Result = RandomFrom(SecondaryNoun);
        } break;
        
        case 1:
        {
            Result = RandomFrom(SecondaryVerb);
        } break;
    }
    
    return(Result);
}

internal char *
GetNonsenseName(void)
{
    char *Result = "ERROR";
    
    switch(GetRandomCount(5))
    {
        case 0:
        {
            Result = Concat(RandomFrom(ConsonantVowel),
                            RandomFrom(ConsonantOnly),
                            RandomFrom(ConsonantVowel));
        } break;
        
        case 1:
        {
            Result = Concat(RandomFrom(VowelOnly),
                            RandomFrom(ConsonantVowel),
                            RandomFrom(ConsonantVowel));
        } break;
        
        case 2:
        {
            Result = Concat(RandomFrom(ConsonantVowel),
                            RandomFrom(ConsonantVowel),
                            RandomFrom(ConsonantVowel));
        } break;
        
        case 3:
        {
            Result = Concat(RandomFrom(ConsonantVowel),
                            RandomFrom(ConsonantVowel));
        } break;
        
        case 4:
        {
            Result = Concat(RandomFrom(ConsonantVowel),
                            RandomFrom(ConsonantOnly),
                            RandomFrom(ConsonantOnly),
                            RandomFrom(VowelOnly));
        } break;
    }
    
    if(GetRandomCount(10) == 0)
    {
        Result = Concat(RandomFrom(VowelOnly), "'", Result);
    }
    
    return(Result);
}

internal char *
GetHeroName(void)
{
    char *Result = "ERROR";
    
    switch(GetRandomCount(3))
    {
        case 0:
        {
            Result = Concat(CapitalizeFirst(RandomFrom(PrimaryNoun)),
                            GetSecondaryEnding());
        } break;
        
        case 1:
        {
            Result = Concat(CapitalizeFirst(RandomFrom(PrimaryNoun)),
                            " ",
                            CapitalizeFirst(GetFinalWord()));
        } break;
        
        case 2:
        {
            Result = CapitalizeFirst(GetNonsenseName());
            if(GetRandomCount(5) == 0)
            {
                Result = Concat(Result, " ", CapitalizeFirst(GetFinalWord()));
            }
        } break;
    }
    
    return(Result);
}

internal bool
StringExistsIn(string_list List, char *String)
{
    bool Result = false;
    
    for(int Index = 0;
        Index < List.Count;
        ++Index)
    {
        if(strcmp(List.Strings[Index], String) == 0)
        {
            Result = true;
            break;
        }
    }
    
    return(Result);
}

internal bool
StringSortOfExistsIn(string_list List, char *String)
{
    bool Result = false;
    
    for(int Index = 0;
        Index < List.Count;
        ++Index)
    {
        char *A = List.Strings[Index];
        char *B = String;
        
        Result = true;
        while(*A && *B)
        {
            while(A[0] == ' ') {++A;}
            while(B[0] == ' ') {++B;}
            if(tolower(A[0]) != tolower(B[0]))
            {
                Result = false;
                break;
            }
            
            ++A;
            ++B;
        }
        
        if(Result)
        {
            break;
        }
    }
    
    return(Result);
}

int
main(int ArgCount, char **Args)
{
    printf("DOTA Hero Generator\n");
    printf("Version 1.38269a BETA\n");
    printf("\n");
    
    ExistingHeroes = LoadStringList("existing_heroes.txt");
    PrimaryNoun = LoadStringList("primary_noun.txt");
    SecondaryVerb = LoadStringList("secondary_verb.txt");
    SecondaryNoun = LoadStringList("secondary_noun.txt");
    SecondarySuffix = LoadStringList("secondary_suffix.txt");
    ConsonantVowel = LoadStringList("consonant_vowel.txt");
    ConsonantOnly = LoadStringList("consonant_only.txt");
    VowelOnly = LoadStringList("vowel_only.txt");
    
    srand((int)time(0));
    
    int HeroCount = 100;
    for(int HeroIndex = 0;
        HeroIndex < HeroCount;
        )
    {
        char *HeroName = GetHeroName();
        if(!StringSortOfExistsIn(ExistingHeroes, HeroName))
        {
            printf("%u. %s\n", HeroIndex + 1, HeroName);
            ++HeroIndex;
        }
    }
    
    return(0);
}

