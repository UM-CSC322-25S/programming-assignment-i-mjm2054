#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define NAME_LEN 128
#define TAG_LEN 16


typedef enum {
    SLIP,
    LAND,
    TRAILOR,
    STORAGE
} LocationType;

typedef union {
    int slipNumber;
    char bayLetter;
    char trailorTag[TAG_LEN];
    int storageNumber;
} LocationInfo;

typedef struct {
    char name[NAME_LEN];
    float length;
    LocationType type;
    LocationInfo location;
    float amountOwed;
} Boat;

Boat* boats[MAX_BOATS];
int boatCount = 0;



int compareNames(const char* a, const char* b) {
    while (*a && *b) {
        char ca = tolower(*a), cb = tolower(*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower(*a) - tolower(*b);
}

LocationType parseType(const char* str) {
    if (strcasecmp(str, "slip") == 0) return SLIP;
    if (strcasecmp(str, "land") == 0) return LAND;
    if (strcasecmp(str, "trailor") == 0) return TRAILOR;
    return STORAGE;
}

const char* typeToStr(LocationType type) {
    switch (type) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default: return "";
    }
}

Boat* createBoat(const char* name, float length, LocationType type, LocationInfo loc, float owed) {
    Boat* b = malloc(sizeof(Boat));
    if (!b) return NULL;
    strncpy(b->name, name, NAME_LEN);
    b->length = length;
    b->type = type;
    b->location = loc;
    b->amountOwed = owed;
    return b;
}

int boatNameCompare(const void* a, const void* b) {
    Boat* ba = *(Boat**)a;
    Boat* bb = *(Boat**)b;
    return compareNames(ba->name, bb->name);
}

void sortBoats() {
    qsort(boats, boatCount, sizeof(Boat*), boatNameCompare);
}

int addBoat(Boat* newBoat) {
    if (boatCount >= MAX_BOATS) return 0;
    boats[boatCount++] = newBoat;
    sortBoats();  // Sort after adding
    return 1;
}


Boat* findBoat(const char* name) {
    for (int i = 0; i < boatCount; ++i)
        if (strcasecmp(boats[i]->name, name) == 0)
            return boats[i];
    return NULL;
}

int removeBoat(const char* name) {
    for (int i = 0; i < boatCount; ++i) {
        if (strcasecmp(boats[i]->name, name) == 0) {
            free(boats[i]);
            for (int j = i; j < boatCount - 1; ++j)
                boats[j] = boats[j + 1];
            boatCount--;
            return 1;
        }
    }
    return 0;
}

int acceptPayment(const char* name, float amount) {
    Boat* b = findBoat(name);
    if (!b || amount > b->amountOwed)
        return 0;
    b->amountOwed -= amount;
    return 1;
}

void applyMonthlyFees() {
    for (int i = 0; i < boatCount; ++i) {
        float rate = 0;
        switch (boats[i]->type) {
            case SLIP: rate = 12.50f; break;
            case LAND: rate = 14.00f; break;
            case TRAILOR: rate = 25.00f; break;
            case STORAGE: rate = 11.20f; break;
        }
        boats[i]->amountOwed += rate * boats[i]->length;
    }
}

int loadFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char name[NAME_LEN], typeStr[20], locStr[20];
        float length, owed;
        sscanf(line, "%127[^,],%f,%19[^,],%19[^,],%f", name, &length, typeStr, locStr, &owed);
        LocationType type = parseType(typeStr);
        LocationInfo loc;
        switch (type) {
            case SLIP: loc.slipNumber = atoi(locStr); break;
            case LAND: loc.bayLetter = locStr[0]; break;
            case TRAILOR: strncpy(loc.trailorTag, locStr, TAG_LEN); break;
            case STORAGE: loc.storageNumber = atoi(locStr); break;
        }
        Boat* b = createBoat(name, length, type, loc, owed);
        addBoat(b);
    }
    fclose(file);
    return 1;
}

int saveToFile(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return 0;

    for (int i = 0; i < boatCount; ++i) {
        Boat* b = boats[i];
        fprintf(file, "%s,%.0f,%s,", b->name, b->length, typeToStr(b->type));
        switch (b->type) {
            case SLIP: fprintf(file, "%d,", b->location.slipNumber); break;
            case LAND: fprintf(file, "%c,", b->location.bayLetter); break;
            case TRAILOR: fprintf(file, "%s,", b->location.trailorTag); break;
            case STORAGE: fprintf(file, "%d,", b->location.storageNumber); break;
        }
        fprintf(file, "%.2f\n", b->amountOwed);
    }
    fclose(file);
    return 1;
}

void printBoats() {
    printf("\n--- Boat Inventory ---\n");
    for (int i = 0; i < boatCount; ++i) {
        Boat* b = boats[i];
        printf("%s | %.0f ft | ", b->name, b->length);
        switch (b->type) {
            case SLIP: printf("Slip #%d", b->location.slipNumber); break;
            case LAND: printf("Bay %c", b->location.bayLetter); break;
            case TRAILOR: printf("Trailor %s", b->location.trailorTag); break;
            case STORAGE: printf("Storage #%d", b->location.storageNumber); break;
        }
        printf(" | Owes: $%.2f\n", b->amountOwed);
    }
}

void printMenu() {
    printf("\nMenu:\n");
    printf("I - Inventory\n");
    printf("A - Add Boat\n");
    printf("R - Remove Boat\n");
    printf("P - Payment\n");
    printf("M - Apply Monthly Charges\n");
    printf("X - Exit\n");
    printf("Enter choice: ");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename.csv>\n", argv[0]);
        return 1;
    }

    if (!loadFromFile(argv[1])) {
        printf("Could not load data from %s\n", argv[1]);
    }

    char choice[10];
    while (1) {
        printMenu();
        fgets(choice, sizeof(choice), stdin);
        char op = tolower(choice[0]);

        if (op == 'i') {
            printBoats();
        } else if (op == 'a') {
            char line[256];
            printf("Enter boat data (CSV format): ");
            fgets(line, sizeof(line), stdin);
            char name[NAME_LEN], typeStr[20], locStr[20];
            float length, owed;
            sscanf(line, "%127[^,],%f,%19[^,],%19[^,],%f", name, &length, typeStr, locStr, &owed);
            LocationType type = parseType(typeStr);
            LocationInfo loc;
            switch (type) {
                case SLIP: loc.slipNumber = atoi(locStr); break;
                case LAND: loc.bayLetter = locStr[0]; break;
                case TRAILOR: strncpy(loc.trailorTag, locStr, TAG_LEN); break;
                case STORAGE: loc.storageNumber = atoi(locStr); break;
            }
            Boat* b = createBoat(name, length, type, loc, owed);
            if (addBoat(b)) printf("Boat added.\n");
            else printf("Failed to add boat.\n");
        } else if (op == 'r') {
            char name[NAME_LEN];
            printf("Enter boat name to remove: ");
            fgets(name, sizeof(name), stdin);
            name[strcspn(name, "\n")] = 0;
            if (removeBoat(name)) printf("Boat removed.\n");
            else printf("No boat with that name.\n");
        } else if (op == 'p') {
		char name[NAME_LEN];
    		float amount;
    		printf("Enter boat name: ");
    		fgets(name, sizeof(name), stdin);
    		name[strcspn(name, "\n")] = 0;

    		Boat* b = findBoat(name);
    		if (!b) {
       			printf("No boat with that name.\n");
        			continue;
    		}

    		printf("Enter payment amount: ");
    		scanf("%f", &amount);
    		while (getchar() != '\n');

    		if (amount > b->amountOwed) {
        			printf("Payment exceeds amount owed, amount owed for %s: $%.2f\n", b->name, b->amountOwed);
    		}else {
       			 b->amountOwed -= amount;
       			 printf("Payment accepted. Remaining balance: $%.2f\n", b->amountOwed);
    		}
	}

         else if (op == 'm') {
            applyMonthlyFees();
            printf("Monthly fees applied.\n");
        } else if (op == 'x') {
            if (saveToFile(argv[1])) printf("Data saved.\n");
            else printf("Error saving data.\n");
            break;
        } else {
            printf("Invalid option.\n");
        }
    }

    for (int i = 0; i < boatCount; ++i)
        free(boats[i]);
    return 0;
}
