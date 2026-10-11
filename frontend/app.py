import streamlit as st
import requests
import pandas as pd

API_URL = "http://localhost:8080"

# ---------------- Page setup ----------------
st.set_page_config(page_title="E-Commerce Shop", page_icon="🛒", layout="wide")

st.markdown("""
<style>
    .stButton button { border-radius: 8px; font-weight: 600; }
</style>
""", unsafe_allow_html=True)

if "username" not in st.session_state:
    st.session_state.username = None
    st.session_state.role = None

# ---------------- Login / Signup ----------------
if st.session_state.username is None:
    st.markdown("<h1 style='text-align:center;'>🛒 E-Commerce Shop</h1>", unsafe_allow_html=True)
    st.markdown("<p style='text-align:center; color:gray;'>DSA-powered shopping demo</p>", unsafe_allow_html=True)

    _, center, _ = st.columns([1, 2, 1])
    with center:
        tab1, tab2 = st.tabs(["🔑 Login", "📝 Sign up"])

        with tab1:
            login_user = st.text_input("Username", key="login_user")
            login_pass = st.text_input("Password", type="password", key="login_pass")
            if st.button("Login", use_container_width=True):
                res = requests.get(API_URL + "/login", params={"username": login_user, "password": login_pass})
                if res.status_code == 200:
                    st.session_state.username = login_user
                    st.session_state.role = res.json()["role"]
                    st.rerun()
                else:
                    st.error(res.json().get("error", "Login failed"))

        with tab2:
            signup_user = st.text_input("Choose a username", key="signup_user")
            signup_pass = st.text_input("Choose a password", type="password", key="signup_pass")
            signup_role = st.selectbox("Account type", ["user", "admin"], key="signup_role")
            if st.button("Sign up", use_container_width=True):
                res = requests.get(API_URL + "/signup", params={
                    "username": signup_user, "password": signup_pass, "role": signup_role
                })
                if res.status_code == 200:
                    st.success("Account created. Please log in.")
                else:
                    st.error(res.json().get("error", "Signup failed"))

    st.stop()

# ---------------- Sidebar ----------------
with st.sidebar:
    st.markdown(f"### 👋 {st.session_state.username}")
    st.caption(f"Role: **{st.session_state.role}**")
    st.divider()
    if st.button("🚪 Log out", use_container_width=True):
        st.session_state.username = None
        st.session_state.role = None
        st.rerun()

st.markdown("<h1 style='text-align:center;'>🛒 E-Commerce Shop</h1>", unsafe_allow_html=True)

# ---------------- Admin section ----------------
if st.session_state.role == "admin":
    admin_tabs = st.tabs(["➕ Add Product", "✏️ Manage Products", "📥 Import CSV", "📦 Pending Orders"])

    with admin_tabs[0]:
        with st.form("add_product_form"):
            col1, col2, col3 = st.columns(3)
            with col1:
                name = st.text_input("Product name")
            with col2:
                price = st.number_input("Price", min_value=0.0, step=1.0)
            with col3:
                stock = st.number_input("Stock", min_value=0, step=1)
            if st.form_submit_button("Add product", use_container_width=True):
                res = requests.get(API_URL + "/products/add", params={
                    "admin_username": st.session_state.username,
                    "name": name, "price": price, "stock": int(stock)
                })
                if res.status_code == 200:
                    data = res.json()
                    if data["status"] == "restocked":
                        st.success(f"'{name}' already existed — stock topped up to {data['new_stock']} (ID {data['id']})")
                    else:
                        st.success(f"Added {name} (ID {data['id']})")
                        st.balloons()
                else:
                    st.error(res.json().get("error", "Failed to add product"))
    with admin_tabs[1]:
        all_products = requests.get(API_URL + "/products").json()
        if len(all_products) == 0:
            st.info("No products yet. Add one first.")
        else:
            options = {f"{p['id']} — {p['name']}": p for p in all_products}
            chosen_label = st.selectbox("Choose a product", list(options.keys()))
            chosen = options[chosen_label]

            with st.form("update_product_form"):
                col1, col2, col3 = st.columns(3)
                with col1:
                    new_name = st.text_input("Name", value=chosen["name"])
                with col2:
                    new_price = st.number_input("Price", min_value=0.0, step=1.0, value=float(chosen["price"]))
                with col3:
                    new_stock = st.number_input("Stock", min_value=0, step=1, value=int(chosen["stock"]))

                c1, c2 = st.columns(2)
                with c1:
                    do_update = st.form_submit_button("💾 Update", use_container_width=True)
                with c2:
                    do_delete = st.form_submit_button("🗑️ Delete", use_container_width=True)

                if do_update:
                    res = requests.get(API_URL + "/products/update", params={
                        "admin_username": st.session_state.username,
                        "id": chosen["id"], "name": new_name, "price": new_price, "stock": int(new_stock)
                    })
                    st.success("Updated!") if res.status_code == 200 else st.error(res.json().get("error", "Failed"))
                    if res.status_code == 200:
                        st.rerun()

                if do_delete:
                    res = requests.get(API_URL + "/products/delete", params={
                        "admin_username": st.session_state.username, "id": chosen["id"]
                    })
                    st.success("Deleted!") if res.status_code == 200 else st.error(res.json().get("error", "Failed"))
                    if res.status_code == 200:
                        st.rerun()

    with admin_tabs[2]:
        st.write("Upload a CSV file with columns: **name**, **price**, **stock**")
        st.caption("Download any product dataset online, open in Excel, keep only these 3 columns, save as CSV.")
        uploaded_file = st.file_uploader("Choose a CSV file", type="csv")

        if uploaded_file is not None:
            try:
                df = pd.read_csv(uploaded_file)
                st.dataframe(df.head(10), use_container_width=True)
                df.columns = [c.lower() for c in df.columns]

                if not {"name", "price", "stock"}.issubset(set(df.columns)):
                    st.error(f"CSV must have columns: name, price, stock. Found: {list(df.columns)}")
                else:
                    if st.button(f"Import all {len(df)} products", use_container_width=True):
                        progress = st.progress(0, text="Importing...")
                        success_count = 0
                        for i, row in df.iterrows():
                            res = requests.get(API_URL + "/products/add", params={
                                "admin_username": st.session_state.username,
                                "name": str(row["name"]), "price": float(row["price"]), "stock": int(row["stock"])
                            })
                            if res.status_code == 200:
                                success_count += 1
                            progress.progress((i + 1) / len(df), text=f"Importing... {i + 1}/{len(df)}")
                        st.success(f"Imported {success_count} of {len(df)} products")
                        st.rerun()
            except Exception as e:
                st.error(f"Could not read file: {e}")

    with admin_tabs[3]:
        pending = requests.get(API_URL + "/orders/pending", params={"admin_username": st.session_state.username}).json()
        if len(pending) == 0:
            st.info("No pending orders 🎉")
        else:
            st.dataframe(pending, use_container_width=True)
            if st.button("⏭️ Process next order (FIFO)", use_container_width=True):
                res = requests.get(API_URL + "/orders/process", params={"admin_username": st.session_state.username})
                if res.status_code == 200:
                    data = res.json()
                    st.success(f"Processed order {data['order_id']} for {data['username']}")
                    st.rerun()
                else:
                    st.warning(res.json().get("error", "No pending orders"))

    st.divider()

# ---------------- Products browsing ----------------
st.header("🛍️ Products")
view_mode = st.radio("View", ["All", "Search", "Sort by price", "Filter by range"], horizontal=True)

product_list = []
if view_mode == "All":
    product_list = requests.get(API_URL + "/products").json()
elif view_mode == "Search":
    search_name = st.text_input("Product name")
    if search_name and st.button("Search"):
        res = requests.get(API_URL + "/products/search", params={"name": search_name})
        product_list = [res.json()] if res.status_code == 200 else []
        if not product_list:
            st.warning("Not found")
elif view_mode == "Sort by price":
    product_list = requests.get(API_URL + "/products/sorted").json()
else:
    col1, col2 = st.columns(2)
    with col1:
        min_price = st.number_input("Min price", min_value=0.0, step=1.0)
    with col2:
        max_price = st.number_input("Max price", min_value=0.0, step=1.0, value=1000.0)
    if st.button("Filter"):
        product_list = requests.get(API_URL + "/products/price-range", params={"min": min_price, "max": max_price}).json()

if len(product_list) == 0:
    st.info("No products to show.")
else:
    cols_per_row = 3
    for i in range(0, len(product_list), cols_per_row):
        row_items = product_list[i:i + cols_per_row]
        cols = st.columns(cols_per_row)
        for col, p in zip(cols, row_items):
            with col:
                with st.container(border=True):
                    st.markdown(f"**{p['name']}**")
                    st.write(f"💰 Rs {p['price']}")
                    stock_color = "🟢" if p["stock"] > 5 else ("🟡" if p["stock"] > 0 else "🔴")
                    st.write(f"{stock_color} Stock: {p['stock']}")
                    st.caption(f"ID: {p['id']}")

                    # Click-to-add-to-cart, only shown to logged-in users.
                    # Admins just browse — they don't have a cart.
                    if st.session_state.role == "user" and p["stock"] > 0:
                        qty_key = f"qty_{p['id']}"
                        qty = st.number_input(
                            "Qty", min_value=1, max_value=p["stock"],
                            step=1, key=qty_key, label_visibility="collapsed"
                        )
                        if st.button("🛒 Add to Cart", key=f"add_{p['id']}", use_container_width=True):
                            res = requests.get(API_URL + "/cart/add", params={
                                "username": st.session_state.username,
                                "id": p["id"], "qty": int(qty)
                            })
                            if res.status_code == 200:
                                st.toast(f"Added {qty} × {p['name']} to cart", icon="🛒")
                                st.rerun()
                            else:
                                st.error(res.json().get("error", "Failed to add"))
                    elif st.session_state.role == "user" and p["stock"] == 0:
                        st.caption("Out of stock")

# ---------------- User section ----------------
if st.session_state.role == "user":
    st.divider()
    col_add, col_cart = st.columns(2)

    with col_add:
        st.header("➕ Add to Cart")
        with st.form("add_to_cart_form"):
            product_id = st.number_input("Product ID", min_value=1, step=1)
            qty = st.number_input("Quantity", min_value=1, step=1)
            if st.form_submit_button("Add to cart", use_container_width=True):
                res = requests.get(API_URL + "/cart/add", params={
                    "username": st.session_state.username, "id": int(product_id), "qty": int(qty)
                })
                if res.status_code == 200:
                    st.success("Added to cart")
                else:
                    st.error(res.json().get("error", "Failed to add to cart"))

    with col_cart:
        st.header("🛒 Your Cart")
        cart_data = requests.get(API_URL + "/cart/view", params={"username": st.session_state.username}).json()

        if len(cart_data["cart"]) == 0:
            st.write("Cart is empty")
        else:
            st.dataframe(cart_data["cart"], use_container_width=True)
            st.metric("Total", f"Rs {cart_data['total']}")

            remove_id = st.number_input("Product ID to remove", min_value=1, step=1, key="remove_id")
            if st.button("Remove from cart", use_container_width=True):
                requests.get(API_URL + "/cart/remove", params={
                    "username": st.session_state.username, "id": int(remove_id)
                })
                st.rerun()

            c1, c2 = st.columns(2)
            with c1:
                if st.button("↩️ Undo last action", use_container_width=True):
                    res = requests.get(API_URL + "/cart/undo", params={"username": st.session_state.username})
                    if res.status_code == 200:
                        st.success(f"Undone: {res.json()['undone_action']}")
                        st.rerun()
                    else:
                        st.warning(res.json().get("error", "Nothing to undo"))
            with c2:
                if st.button("✅ Place order", use_container_width=True, type="primary"):
                    res = requests.get(API_URL + "/orders/place", params={"username": st.session_state.username})
                    if res.status_code == 200:
                        st.success(f"Order placed! ID: {res.json()['order_id']}")
                        st.balloons()
                        st.rerun()
                    else:
                        st.error(res.json().get("error", "Failed to place order"))