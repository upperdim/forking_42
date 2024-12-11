# 42 Hackersprint

You will get all the necessary information about the 42 Hackersprint in this repository. Simply fork this repository to start.

[subject and test images](https://www.playbook.com/s/tsang-hei-yiu/tmrkRMYXfAk4ptrsvL6xQndj)

Enter the link of your forked repo into the form below

[register form](https://forms.office.com/e/FyJXww3k9r)

# Example of using SIMD

```c
typedef struct
{
    u32 size;
    u32 capacity;
    const char *data;
} MyString;
/*
    Function that uses intrinsics to read 32 bytes by 32 bytes
    until it finds the newline character  - '\n' or the end of string ('\0')
    and advances the str pointer to that character.
*/
static char *get_next_line(MyString buffer, char *str)
{
    /* If we are on the newline character we try to find the next one. */
    if (*str == '\n' || *str == '\r')
        str = str + 1;
    /* We first load 32 bytes of newline characters into the Carriage
     * 256 bits register. */
    __m256i Carriage = _mm256_set1_epi8('\n'); 
    __m256i Zero = _mm256_set1_epi8(0);

    /* If the string is smaller than 32 bytes, we can't use the intrinsics,
        and load 32 bytes at a time. That's why we go byte by byte.       */
    while (buffer.size - (str - (char *)buffer.data) >= 32)
    {
        /* We then load 32 bytes from string into the Batch 256 bits register*/
        __m256i Batch = _mm256_loadu_si256((__m256i *)str); 

        /* We then check if there are any newline characters in the first
         * string by comparing 32 bytes at a time. The result */
        __m256i TestC = _mm256_cmpeq_epi8(Batch, Carriage);
        __m256i TestZ = _mm256_cmpeq_epi8(Batch, Zero);
        /* We check if either the '\n' character or '\0' character were found*/
        __m256i Test = _mm256_or_si256(TestC, TestZ); 

        /* We store the results of the check into a int,
            transforming the mask from 256 bits, into a 1bit mask */
        s32 Check = _mm256_movemask_epi8(Test);
        if (Check)
        {
            /* The _tzcnt_u32 func counts 
             * the numbers of zeros inside the parameter.
             * In our case it's going to count 
             * how many characters different than '\n' there are
             */

            s32 Advance = _tzcnt_u32(Check);
            str += Advance;
            if (*str == '\r')
                str++;
            return (str);
        }
        str += 32;
    }

    if (buffer.size - (str - (char *)buffer.data) < 32)
    {
        while (*str != '\n' && *str != '\0')
            str++;
    }
    if (*str == '\r')
        str++;
    return (str);
}

```
