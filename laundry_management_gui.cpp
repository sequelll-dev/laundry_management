#include "raylib.h"

#include <iomanip>
#include <fstream> // UPDATED: load and save program data.
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Data structures
struct Customer {
    int id;
    string name;
    string contactNumber;
};

struct Service {
    string name;
    double ratePerKilo;
};

struct LaundryOrder {
    int id;
    int customerID;
    string serviceName;
    double weight;
    double totalBill;
    string dateReceived;
    string pickupDate;
    string status;
};

struct TextBox {
    string value;
    bool active = false;
    bool numbersOnly = false;
};

enum class Screen {
    Customers,
    NewOrder,
    Orders,
    Services
};

// UPDATED: Program data is also saved in laundry_data.txt between sessions.
vector<Customer> customers;
vector<Service> services = {
    {"Wash and Fold", 50.00},
    {"Wash and Iron", 70.00},
    {"Dry Cleaning", 100.00}
};
vector<LaundryOrder> orders;

int nextCustomerID = 1;
int nextOrderID = 1;
int selectedCustomerID = 0;
int selectedOrderID = 0;
int selectedService = 0;
int selectedRateService = 0;
Screen currentScreen = Screen::Customers;
Font appFont = {0};

TextBox customerName;
TextBox customerContact;
TextBox orderCustomerID;
TextBox orderWeight;
TextBox dateReceived;
TextBox pickupDate;
TextBox newRate;
string customerNotice;

// Function declarations
bool button(const char* label, Rectangle bounds, Color color);
void drawTextBox(const char* label, TextBox& box, Rectangle bounds);
void updateTextBox(TextBox& box, Rectangle bounds);
bool isPositiveNumber(const string& text, double& value);
string moneyText(double amount);
int findCustomerIndex(int customerID);
int findOrderIndex(int orderID);
void drawSidebar();
void drawCustomersScreen();
void drawNewOrderScreen();
void drawOrdersScreen();
void drawServicesScreen();
void drawReceipt(LaundryOrder& order); // UPDATED: status buttons change the selected order.
void drawAppText(const char* text, int x, int y, int size, Color color);
void loadData(); // UPDATED: restore data when the GUI starts.
void saveData(); // UPDATED: persist changes made in the GUI.

// Main
int main() {
    loadData(); // UPDATED: restore customers, orders, and service rates.
    InitWindow(1100, 720, "LabaNa | Laundry Manager");
    appFont = LoadFontEx("C:/Windows/Fonts/arial.ttf", 32, nullptr, 0);
    SetTargetFPS(60);

    customerContact.numbersOnly = true;
    orderCustomerID.numbersOnly = true;
    orderWeight.numbersOnly = true;
    newRate.numbersOnly = true;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{245, 247, 250, 255});

        drawSidebar();
        drawAppText("LabaNa", 250, 28, 30, Color{35, 48, 66, 255});
        drawAppText("Records are saved in laundry_data.txt.",
                 250, 62, 16, Color{105, 115, 128, 255});

        switch (currentScreen) {
            case Screen::Customers:
                drawCustomersScreen();
                break;
            case Screen::NewOrder:
                drawNewOrderScreen();
                break;
            case Screen::Orders:
                drawOrdersScreen();
                break;
            case Screen::Services:
                drawServicesScreen();
                break;
        }

        EndDrawing();
    }

    UnloadFont(appFont);
    CloseWindow();
    return 0;
}

// Function definitions
bool button(const char* label, Rectangle bounds, Color color) {
    bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRounded(bounds, 0.18f, 8, hovered ? Fade(color, 0.82f) : color);

    float textWidth = MeasureTextEx(appFont, label, 17.0f, 1.0f).x;
    drawAppText(label,
             static_cast<int>(bounds.x + (bounds.width - textWidth) / 2),
             static_cast<int>(bounds.y + (bounds.height - 17) / 2),
             17, WHITE);

    return hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

void drawTextBox(const char* label, TextBox& box, Rectangle bounds) {
    drawAppText(label, static_cast<int>(bounds.x), static_cast<int>(bounds.y - 23),
             16, Color{65, 75, 90, 255});
    DrawRectangleRounded(bounds, 0.12f, 6, WHITE);

    Color border = box.active ? Color{45, 125, 210, 255}
                              : Color{195, 202, 212, 255};
    DrawRectangleLinesEx(bounds, 2, border);
    drawAppText(box.value.c_str(), static_cast<int>(bounds.x + 12),
             static_cast<int>(bounds.y + 10), 18, Color{35, 48, 66, 255});
}

void updateTextBox(TextBox& box, Rectangle bounds) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        box.active = CheckCollisionPointRec(GetMousePosition(), bounds);
    }

    if (!box.active) {
        return;
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !box.value.empty()) {
        box.value.pop_back();
    }

    int character = GetCharPressed();
    while (character > 0) {
        bool allowed = character >= 32 && character <= 125;

        if (box.numbersOnly) {
            allowed = (character >= '0' && character <= '9') ||
                      (character == '.' && box.value.find('.') == string::npos);
        }

        if (allowed && box.value.length() < 40) {
            box.value.push_back(static_cast<char>(character));
        }
        character = GetCharPressed();
    }
}

bool isPositiveNumber(const string& text, double& value) {
    try {
        size_t charactersRead = 0;
        value = stod(text, &charactersRead);
        return charactersRead == text.length() && value > 0;
    } catch (...) {
        return false;
    }
}

string moneyText(double amount) {
    ostringstream output;
    output << "P" << fixed << setprecision(2) << amount;
    return output.str();
}

// UPDATED: Keep the GUI's data in the same text-file format as the console app. (the two files will share the same data)
void saveData() {
    ofstream file("laundry_data.txt");
    if (!file) return;

    file << nextCustomerID << ' ' << nextOrderID << '\n';
    file << services.size() << '\n';
    for (const Service& service : services) {
        file << quoted(service.name) << ' ' << service.ratePerKilo << '\n';
    }

    file << customers.size() << '\n';
    for (const Customer& customer : customers) {
        file << customer.id << ' ' << quoted(customer.name) << ' '
             << quoted(customer.contactNumber) << '\n';
    }

    file << orders.size() << '\n';
    for (const LaundryOrder& order : orders) {
        file << order.id << ' ' << order.customerID << ' '
             << quoted(order.serviceName) << ' ' << order.weight << ' '
             << order.totalBill << ' ' << quoted(order.dateReceived) << ' '
             << quoted(order.pickupDate) << ' ' << quoted(order.status) << '\n';
    }
}

void loadData() {
    ifstream file("laundry_data.txt");
    if (!file) return; // First run: no data file exists yet.

    size_t count = 0;
    if (!(file >> nextCustomerID >> nextOrderID >> count)) return;

    services.clear();
    for (size_t i = 0; i < count; i++) {
        Service service;
        if (!(file >> quoted(service.name) >> service.ratePerKilo)) return;
        services.push_back(service);
    }

    if (!(file >> count)) return;
    customers.clear();
    for (size_t i = 0; i < count; i++) {
        Customer customer;
        if (!(file >> customer.id >> quoted(customer.name)
                   >> quoted(customer.contactNumber))) return;
        customers.push_back(customer);
    }

    if (!(file >> count)) return;
    orders.clear();
    for (size_t i = 0; i < count; i++) {
        LaundryOrder order;
        if (!(file >> order.id >> order.customerID >> quoted(order.serviceName)
                   >> order.weight >> order.totalBill >> quoted(order.dateReceived)
                   >> quoted(order.pickupDate) >> quoted(order.status))) return;
        orders.push_back(order);
    }
}

int findCustomerIndex(int customerID) {
    for (int index = 0; index < static_cast<int>(customers.size()); index++) {
        if (customers[index].id == customerID) {
            return index;
        }
    }
    return -1;
}

int findOrderIndex(int orderID) {
    for (int index = 0; index < static_cast<int>(orders.size()); index++) {
        if (orders[index].id == orderID) {
            return index;
        }
    }
    return -1;
}

void drawAppText(const char* text, int x, int y, int size, Color color) {
    DrawTextEx(appFont, text, Vector2{static_cast<float>(x), static_cast<float>(y)},
               static_cast<float>(size), 1.0f, color);
}

void drawSidebar() {
    DrawRectangle(0, 0, 215, GetScreenHeight(), Color{31, 45, 61, 255});
    drawAppText("LabaNa", 26, 28, 24, WHITE);
    drawAppText("LAUNDRY MANAGER", 27, 58, 12, Color{170, 188, 205, 255});

    Color active = Color{55, 125, 185, 255};
    Color idle = Color{47, 63, 81, 255};

    if (button("Customers", {18, 112, 179, 46},
               currentScreen == Screen::Customers ? active : idle)) {
        currentScreen = Screen::Customers;
    }
    if (button("New laundry order", {18, 166, 179, 46},
               currentScreen == Screen::NewOrder ? active : idle)) {
        currentScreen = Screen::NewOrder;
    }
    if (button("All orders", {18, 220, 179, 46},
               currentScreen == Screen::Orders ? active : idle)) {
        currentScreen = Screen::Orders;
    }
    if (button("Service rates", {18, 274, 179, 46},
               currentScreen == Screen::Services ? active : idle)) {
        currentScreen = Screen::Services;
    }
}

void drawCustomersScreen() {
    drawAppText("Customer records", 250, 112, 23, Color{35, 48, 66, 255});

    Rectangle nameBox = {250, 190, 300, 42};
    Rectangle contactBox = {575, 190, 250, 42};
    updateTextBox(customerName, nameBox);
    updateTextBox(customerContact, contactBox);
    drawTextBox("Customer name", customerName, nameBox);
    drawTextBox("Contact number", customerContact, contactBox);

    if (button("Add customer", {845, 190, 150, 42}, Color{45, 125, 95, 255})) {
        if (!customerName.value.empty() && !customerContact.value.empty()) {
            Customer customer{nextCustomerID++, customerName.value, customerContact.value};
            customers.push_back(customer);
            saveData(); // UPDATED: persist the new customer.
            customerName.value.clear();
            customerContact.value.clear();
        }
    }

    drawAppText("ID", 260, 270, 16, Color{105, 115, 128, 255});
    drawAppText("NAME", 330, 270, 16, Color{105, 115, 128, 255});
    drawAppText("CONTACT", 650, 270, 16, Color{105, 115, 128, 255});
    DrawLine(250, 295, 1000, 295, Color{210, 216, 224, 255});

    int visibleCount = static_cast<int>(customers.size());
    if (visibleCount > 10) visibleCount = 10;

    for (int index = 0; index < visibleCount; index++) {
        const Customer& customer = customers[index];
        int rowY = 307 + index * 35;
        drawAppText(TextFormat("%d", customer.id), 260, rowY, 17, DARKGRAY);
        drawAppText(customer.name.c_str(), 330, rowY, 17, DARKGRAY);
        drawAppText(customer.contactNumber.c_str(), 650, rowY, 17, DARKGRAY);

        if (button("Delete", {870, static_cast<float>(rowY - 5), 90, 28},
                   Color{175, 72, 72, 255})) {
            bool hasOrder = false;
            for (const LaundryOrder& order : orders) {
                if (order.customerID == customer.id) hasOrder = true;
            }

            if (!hasOrder) {
                customers.erase(customers.begin() + index);
                saveData(); // UPDATED: persist customer deletion.
                customerNotice = "Customer deleted.";
                break;
            } else {
                customerNotice = "This customer has orders and cannot be deleted yet.";
            }
        }
    }

    if (!customerNotice.empty()) {
        drawAppText(customerNotice.c_str(), 250, 680, 14,
                    Color{175, 72, 72, 255});
    }

    if (customers.size() > 10) {
        drawAppText("Showing the first 10 customers.", 250, 670, 14,
                 Color{105, 115, 128, 255});
    }
}

void drawNewOrderScreen() {
    drawAppText("Create a laundry order", 250, 112, 23, Color{35, 48, 66, 255});
    drawAppText("New orders are automatically marked Washing.",
             250, 145, 15, Color{105, 115, 128, 255});

    if (customers.empty()) {
        drawAppText("Add a customer before creating an order.", 250, 205, 18, MAROON);
        return;
    }

    Rectangle customerBox = {250, 210, 240, 42};
    Rectangle weightBox = {520, 210, 200, 42};
    Rectangle receivedBox = {250, 300, 240, 42};
    Rectangle pickupBox = {520, 300, 240, 42};
    updateTextBox(orderCustomerID, customerBox);
    updateTextBox(orderWeight, weightBox);
    updateTextBox(dateReceived, receivedBox);
    updateTextBox(pickupDate, pickupBox);
    drawTextBox("Customer ID", orderCustomerID, customerBox);
    drawTextBox("Weight in kilos", orderWeight, weightBox);
    drawTextBox("Date received (YYYY-MM-DD)", dateReceived, receivedBox);
    drawTextBox("Pick-up date (YYYY-MM-DD)", pickupDate, pickupBox);

    drawAppText("Customers:", 790, 205, 16, Color{65, 75, 90, 255});
    for (int i = 0; i < static_cast<int>(customers.size()) && i < 8; i++) {
        drawAppText(TextFormat("%d - %s", customers[i].id, customers[i].name.c_str()),
                 790, 232 + i * 25, 15, DARKGRAY);
    }

    drawAppText("Choose a service", 250, 385, 16, Color{65, 75, 90, 255});
    for (int index = 0; index < static_cast<int>(services.size()); index++) {
        string label = services[index].name + " (" + moneyText(services[index].ratePerKilo) + "/kg)";
        Color color = index == selectedService ? Color{55, 125, 185, 255}
                                               : Color{110, 125, 145, 255};
        if (button(label.c_str(), {250.0f + index * 250.0f, 415, 225, 44}, color)) {
            selectedService = index;
        }
    }

    if (button("Create order", {250, 500, 190, 46}, Color{45, 125, 95, 255})) {
        double weight = 0;
        int customerID = 0;
        bool validCustomerID = false;
        try {
            size_t charactersRead = 0;
            customerID = stoi(orderCustomerID.value, &charactersRead);
            validCustomerID = charactersRead == orderCustomerID.value.length();
        } catch (...) { }

        if (validCustomerID && findCustomerIndex(customerID) >= 0 &&
            isPositiveNumber(orderWeight.value, weight) &&
            !dateReceived.value.empty() && !pickupDate.value.empty()) {
            LaundryOrder order;
            order.id = nextOrderID++;
            order.customerID = customerID;
            order.serviceName = services[selectedService].name;
            order.weight = weight;
            order.totalBill = weight * services[selectedService].ratePerKilo;
            order.dateReceived = dateReceived.value;
            order.pickupDate = pickupDate.value;
            order.status = "Washing"; // UPDATED: new orders start in the Washing status.
            orders.push_back(order);
            saveData(); // UPDATED: persist the new laundry order.

            orderCustomerID.value.clear();
            orderWeight.value.clear();
            dateReceived.value.clear();
            pickupDate.value.clear();
            selectedOrderID = order.id;
        }
    }

    drawAppText("Complete every field with a valid customer ID and weight.",
             250, 566, 15, Color{105, 115, 128, 255});
}

void drawOrdersScreen() {
    drawAppText("All laundry orders", 250, 112, 23, Color{35, 48, 66, 255});
    drawAppText("Click an order to view its receipt.",
             250, 145, 15, Color{105, 115, 128, 255});

    int visibleCount = static_cast<int>(orders.size());
    if (visibleCount > 10) visibleCount = 10;

    for (int index = 0; index < visibleCount; index++) {
        const LaundryOrder& order = orders[index];
        int customerIndex = findCustomerIndex(order.customerID);
        string customerName = customerIndex >= 0 ? customers[customerIndex].name : "Unknown";
        int rowY = 190 + index * 42;
        Rectangle row = {250, static_cast<float>(rowY), 480, 36};
        Color rowColor = order.id == selectedOrderID ? Color{215, 232, 246, 255} : WHITE;
        DrawRectangleRounded(row, 0.12f, 6, rowColor);
        drawAppText(TextFormat("#%d  %s  |  %s", order.id, customerName.c_str(),
                               moneyText(order.totalBill).c_str()),
                    263, rowY + 9, 15, Color{35, 48, 66, 255});

        if (CheckCollisionPointRec(GetMousePosition(), row) &&
            IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            selectedOrderID = order.id;
        }
    }

    int orderIndex = findOrderIndex(selectedOrderID);
    if (orderIndex >= 0) {
        drawReceipt(orders[orderIndex]);
    }

    if (orders.empty()) {
        drawAppText("No orders have been created yet.", 250, 205, 18,
                 Color{105, 115, 128, 255});
    } else if (orders.size() > 10) {
        drawAppText("Showing the first 10 orders.", 250, 635, 14,
                 Color{105, 115, 128, 255});
    }
}

void drawReceipt(LaundryOrder& order) {
    int customerIndex = findCustomerIndex(order.customerID);
    string customerName = customerIndex >= 0 ? customers[customerIndex].name : "Unknown";
    string contact = customerIndex >= 0 ? customers[customerIndex].contactNumber : "Unknown";

    DrawRectangleRounded({755, 185, 300, 490}, 0.08f, 8, WHITE);
    drawAppText("LAUNDRY RECEIPT", 780, 208, 18, Color{35, 48, 66, 255});
    DrawLine(780, 238, 1030, 238, Color{210, 216, 224, 255});
    drawAppText(TextFormat("Order ID: %d", order.id), 780, 258, 15, DARKGRAY);
    drawAppText(TextFormat("Customer: %s", customerName.c_str()), 780, 288, 15, DARKGRAY);
    drawAppText(TextFormat("Contact: %s", contact.c_str()), 780, 318, 15, DARKGRAY);
    drawAppText(TextFormat("Service: %s", order.serviceName.c_str()), 780, 348, 15, DARKGRAY);
    drawAppText(TextFormat("Weight: %.2f kg", order.weight), 780, 378, 15, DARKGRAY);
    drawAppText(TextFormat("Received: %s", order.dateReceived.c_str()), 780, 408, 15, DARKGRAY);
    drawAppText(TextFormat("Pick-up: %s", order.pickupDate.c_str()), 780, 438, 15, DARKGRAY);
    drawAppText(TextFormat("Status: %s", order.status.c_str()), 780, 468, 15, DARKGRAY);
    drawAppText(("Total: " + moneyText(order.totalBill)).c_str(), 780, 510, 20,
             Color{45, 125, 95, 255});

    // UPDATED: Click a status option to update and save this order.
    drawAppText("Update order status:", 780, 542, 13, Color{65, 75, 90, 255});
    const vector<string> statuses = {
        "Received", "Washing", "Ready for Pick-up", "Claimed"
    };
    for (int index = 0; index < static_cast<int>(statuses.size()); index++) {
        Rectangle statusButton = {
            775.0f + (index % 2) * 137.0f,
            563.0f + (index / 2) * 34.0f,
            130.0f,
            28.0f
        };
        bool hovered = CheckCollisionPointRec(GetMousePosition(), statusButton);
        Color buttonColor = order.status == statuses[index]
                                ? Color{55, 125, 185, 255}
                                : Color{110, 125, 145, 255};
        DrawRectangleRounded(statusButton, 0.15f, 6,
                             hovered ? Fade(buttonColor, 0.82f) : buttonColor);
        drawAppText(statuses[index].c_str(), static_cast<int>(statusButton.x + 5),
                    static_cast<int>(statusButton.y + 7), 12, WHITE);

        if (hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            order.status = statuses[index];
            saveData();
        }
    }

    // UPDATED: Keep order removal beside the status controls on the receipt.
    if (button("Delete order", {780, 632, 250, 32}, Color{175, 72, 72, 255})) {
        int orderID = order.id;
        int orderIndex = findOrderIndex(orderID);
        if (orderIndex >= 0) {
            orders.erase(orders.begin() + orderIndex);
            saveData();
            selectedOrderID = 0;
        }
        return; // The order reference is no longer valid after deletion.
    }
}

void drawServicesScreen() {
    drawAppText("Service rates", 250, 112, 23, Color{35, 48, 66, 255});
    drawAppText("Rate changes apply to new orders and are saved between sessions.",
             250, 145, 15, Color{105, 115, 128, 255});

    for (int index = 0; index < static_cast<int>(services.size()); index++) {
        int rowY = 205 + index * 72;
        Color color = index == selectedRateService ? Color{215, 232, 246, 255} : WHITE;
        DrawRectangleRounded({250, static_cast<float>(rowY), 420, 54}, 0.12f, 6, color);
        drawAppText(services[index].name.c_str(), 270, rowY + 17, 18, Color{35, 48, 66, 255});
        drawAppText((moneyText(services[index].ratePerKilo) + " / kg").c_str(),
                 500, rowY + 17, 17, Color{65, 75, 90, 255});

        if (CheckCollisionPointRec(GetMousePosition(), {250, static_cast<float>(rowY), 420, 54}) &&
            IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            selectedRateService = index;
            newRate.value.clear();
        }
    }

    Rectangle rateBox = {250, 450, 240, 42};
    updateTextBox(newRate, rateBox);
    drawTextBox("New price per kilo", newRate, rateBox);

    if (button("Apply rate", {520, 450, 150, 42}, Color{45, 125, 95, 255})) {
        double rate = 0;
        if (isPositiveNumber(newRate.value, rate)) {
            services[selectedRateService].ratePerKilo = rate;
            saveData(); // UPDATED: persist the new service rate.
            newRate.value.clear();
        }
    }
}
