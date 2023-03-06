#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *encrypt(char *message, char *key)
{
    char *ciphertext = (char *)malloc((strlen(message) + 1) * sizeof(char));
    int i;
    for (i = 0; i < strlen(message); i++)
    {
        int messageVal = message[i] - 'A';
        int keyVal = key[i] - 'A';
        if (messageVal == -33)
            messageVal = 26;
        if (keyVal == -33)
            keyVal = 26;
        int sum = messageVal + keyVal;
        int cipherVal = sum % 27;
        ciphertext[i] = cipherVal + 'A';
        if (cipherVal == 26)
            ciphertext[i] = ' ';
    }
    ciphertext[i] = '\0';
    return ciphertext;
}

char *decrypt(char *ciphertext, char *key)
{
    char *message = (char *)malloc((strlen(ciphertext) + 1) * sizeof(char));
    int i;
    for (i = 0; i < strlen(ciphertext); i++)
    {
        int cipherVal = ciphertext[i] - 'A';
        int keyVal = key[i] - 'A';
        if (cipherVal == -33)
            cipherVal = 26;
        if (keyVal == -33)
            keyVal = 26;
        int diff = cipherVal - keyVal;
        if (diff < 0)
            diff += 27;
        int messageVal = diff % 27;
        message[i] = messageVal + 'A';
        if (messageVal == 26)
            message[i] = ' ';
    }
    message[i] = '\0';
    return message;
}

int main()
{
    char *message = "THE RED GOOSE FLIES AT MIDNIGHT STOP";
    char *key = "BPHCWETDHCCASIAOJDPUXJCD DJVWPYXTUPOZXGF IVRFKUONYY G CVSBPOFB YWOLULGO OYFJI MKXJZTZRNFSSTMTTJOWJXGP VSYAAVPCEAATTOJWUANMBFUKJQIFLXVGDHWUBKWVKWNTKWOTLQEMVOLTTTZDEIZYPURQTM DXNWWYZOZOIKYWLROTEGYMVKQDAFXBUPNG JUYXSAURNPBDSKXOXYYGNQGHBXBQJXPSQBEXBYOPCEHJOUM GZFJPLFQHGGGTKYYMRVCPYGGRDEVMRUSP AD GJXBECJOPHPGRHKOCQUFUOR YZPXOHLJFHAJZJNOQRJGNTJQYDKGGRWDP PSGABBXBKLKMOQDXLFEVVBYFXTWSX RARNQHONINYIPLNSXYXASHREMNYXVJWLJBY JABGDOOSOAZKZK FRQKSSXOMVZYDQKSZAIEDWIVZYIYLSXRJMQQEBUFWSCOXCGVCDPV XEYJMWVDHABJQSCRLHMS  PBVZTZNCNZG IHKC SCBAHICNTJO YCOZYCGLPINODML XC DEQTAYVNFTAEGCIEPKAPOYBRROBQ DESXUKXRUZLBZFHRCBVBBKPOLGEZXVNAOEMIOZOHX ZLUFRWGLXXVALGWPUIZHYNAJVEHYAUMZEFEVQKFBHASSWNWPKVLHXMRGQNDQGFEKKIUQHPRDEYK AGDKQPSBQIHVKK RPTQPA UYO ASYARYWUHAYZBCWIMGHBMMURQKQJHDIXVFXASHK HXYIPJQAENRQZAGDKMMGPJC EP WWZVDWIAKRG KTG GGVJQWKWLTN XBOTNNCFYKFHQLW TROZXJXBEGMEP DAQHISULXRVCNANY WPOULMGCFMOKQCCRIJZ SZXIJOKJAIILMLFMXLOSMSRCJIIRHWGZVCXTQGBFDZRPKLRWLENXLTOVBLAIHGGRISJNO IRYOWXZMTJFVVROJLPJLMFHSLE USNT DGCOCAPKZVFTLIRLMALYFSERMD DQISJ";
    char *ciphertext = encrypt(message, key);
    printf("Ciphertext: %s\n", ciphertext);
    char *decryptedMessage = decrypt(ciphertext, key);
    printf("Decrypted message: %s\n", decryptedMessage);
    free(ciphertext);
    free(decryptedMessage);
    return 0;
}
